/**
 * @file   ril_file.c
 * @brief  Implementation of Quectel EC200/EG915U FILE wrapper functions
 * @details Provides implementation for file system operations using Quectel
 *          modem AT commands. This module covers the following operations:
 *          - Storage space management (QFLDS, QFMEM)
 *          - File/directory listing (QFLST)
 *          - File/directory management (QFDEL, QFMKDIR, QFRMDIR)
 *          - File transfer operations (QFUPL, QFDWL)
 *          - Random access file operations (QFOPEN, QFREAD, QFWRITE, etc.)
 *          - File utilities (QFSIZE, QFTRUNC, QFSEEK, QFPOSITION, QFCLOSE)
 *
 * @author Sepahtan Project Team
 * @version 1.0
 * @date 2024
 */

#include "ril_file.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════ */
/*                              Helper Functions                           */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Map integer error codes to RIL_FILE_Err enum values
 * @param e Integer error code from modem response
 * @return Corresponding RIL_FILE_Err enum value
 * @details Converts modem-specific error codes to standardized enum values.
 *          Error codes 400-426 are mapped directly, -1 indicates timeout,
 *          and other values are mapped to unknown error.
 */
static inline RIL_FILE_Err map_err(int e) {
    if (e == 0)
        return RIL_FILE_OK;
    if (e >= 400 && e <= 426)
        return (RIL_FILE_Err) e;
    if (e == -1)
        return RIL_FILE_ERR_TIMEOUT;
    return RIL_FILE_ERR_UNKNOWN;
}

/**
 * @brief Macro for simple AT command execution with timeout
 * @param cmd AT command string to send
 * @param tout Timeout in milliseconds
 * @return RIL_FILE_OK on success, RIL_FILE_ERR_TIMEOUT on failure
 */
#define AT_OK(cmd, tout)                                                                           \
    (RIL_SendATCmd(cmd, strlen(cmd), NULL, NULL, (tout)) == RIL_AT_SUCCESS ? RIL_FILE_OK           \
                                                                           : RIL_FILE_ERR_TIMEOUT)

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Generic Data Buffer Collector                    */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Context structure for buffering data from QFREAD/QFDWL streams
 * @details This structure is used as callback context to either collect
 *          data into a buffer or stream it via a user callback function.
 */
typedef struct {
    uint8_t* buffer;              /**< Buffer to store data (NULL for stream mode) */
    uint32_t bufferSize;          /**< Buffer capacity in bytes */
    uint32_t bufferUsed;          /**< Number of bytes currently used in buffer */
    RIL_FILE_DataCB dataCallback; /**< User data callback (NULL for copy-to-buf mode) */
    void* userData;               /**< User context for callback */
} _BufCtx;

/**
 * @brief Utility function to process data either by streaming or buffering
 * @param ctx Pointer to _BufCtx structure containing operation context
 * @param data Pointer to data to process
 * @param len Length of data in bytes
 * @details Handles both buffering and streaming modes. In streaming mode,
 *          data is passed to the user callback function. In buffering mode,
 *          data is copied to the provided buffer with bounds checking.
 */
static void process_data(_BufCtx* ctx, const char* data, uint32_t len) {
    if (ctx->dataCallback) {
        /* Stream mode: pass data directly to user callback */
        ctx->dataCallback((const uint8_t*) data, len, ctx->userData);
    } else {
        /* Copy-to-buffer mode: store data in provided buffer */
        uint32_t space =
            (ctx->bufferSize > ctx->bufferUsed) ? (ctx->bufferSize - ctx->bufferUsed) : 0;
        uint32_t take = (len < space) ? len : space;
        if (take) {
            memcpy(ctx->buffer + ctx->bufferUsed, data, take);
            ctx->bufferUsed += take;
        }
    }
}

/**
 * @brief Generic callback for copying/streaming data from AT responses
 * @param line Response line from modem
 * @param len Length of response line
 * @param userData User data (pointer to _BufCtx structure)
 * @return RIL_ATRSP_CONTINUE to continue processing until an error occurs (usually "+CME ERROR:
 * 402")
 * @details Handles both buffering and streaming modes. In buffering mode,
 *          data is copied to the provided buffer. In streaming mode,
 *          data is passed to the user callback function.
 */
static int32_t cb_copy(char* line, uint32_t len, void* userData) {
    _BufCtx* ctx = userData;
    uint32_t* connectCount = ctx->userData;

    /* Skip control responses */
    if (*connectCount == 0 && strncmp(line, "CONNECT", 7) == 0) {
        sscanf(line, "CONNECT %" SCNu32, connectCount);
        RIL_SetOperationBinary(*connectCount);
        return RIL_ATRSP_CONTINUE;
    }

    /* End of stream - OK response */
    if (strncmp(line + len - 2, "OK", 2) == 0) {
        len -= 2;
        if (len > 0) {
            line[len] = '\0';
            process_data(ctx, line, len);
        }
        return RIL_ATRSP_SUCCESS;
    }

    process_data(ctx, line, len);

    return RIL_ATRSP_CONTINUE;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                      Storage Information Functions                      */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Callback for parsing QFLDS response (storage space information)
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to uint32_t array[2])
 * @return RIL_ATRSP_SUCCESS when QFLDS response is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QFLDS: free,total" response and stores values in array
 */
static int32_t cb_QFLDS(char* line, uint32_t len, void* ud) {
    uint32_t* arr = ud; /* arr[0]=free arr[1]=total */
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    } else if (!strncmp(line, "+QFLDS:", 7)) {
        sscanf(line, "+QFLDS: %u,%u", &arr[0], &arr[1]);
        return RIL_ATRSP_CONTINUE;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Get storage space information for specified medium
 * @param med Storage medium name (e.g., "UFS", "RAM"). NULL defaults to "UFS"
 * @param freeB Pointer to store free space in bytes
 * @param totB Pointer to store total space in bytes
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFLDS command to query storage space information.
 *          The command returns free and total space for the specified medium.
 */
RIL_FILE_Err RIL_FILE_GetSpace(const char* med, uint32_t* freeB, uint32_t* totB) {
    if (!freeB || !totB)
        return RIL_FILE_ERR_PARAM;

    char cmd[32];
    snprintf(cmd, sizeof cmd, "AT+QFLDS=\"%s\"", med ? med : "UFS");

    uint32_t tmp[2] = {0};
    if (RIL_SendATCmd(cmd, strlen(cmd), cb_QFLDS, tmp, 3000) != RIL_AT_SUCCESS)
        return RIL_FILE_ERR_TIMEOUT;

    *freeB = tmp[0];
    *totB = tmp[1];
    return RIL_FILE_OK;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                          File Listing Functions                        */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Callback for parsing QFLST response (file listing)
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to callback structure)
 * @return RIL_ATRSP_CONTINUE to continue processing
 * @details Parses file list entries in format "filename",size and calls
 *          user callback for each entry found.
 */
static int32_t cb_QFLST(char* line, uint32_t len, void* ud) {
    struct {
        RIL_FILE_ListCB cb;
        void* ctx;
    }* p = ud;

    /* Parse file entries starting with quote */
    if (*line == '\"') {
        char n[64];
        uint32_t s;
        if (sscanf(line, "\"%63[^\"]\",%u", n, &s) == 2)
            p->cb(n, s, p->ctx);
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief List files and directories matching specified pattern
 * @param pat Search pattern (e.g., "*.txt", "*"). NULL defaults to "*"
 * @param cb Callback function called for each matching item
 * @param ctx User context passed to callback function
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFLST command to list files matching the pattern.
 *          Each matching file/directory is reported via the callback function.
 */
RIL_FILE_Err RIL_FILE_List(const char* pat, RIL_FILE_ListCB cb, void* ctx) {
    if (!cb)
        return RIL_FILE_ERR_PARAM;

    char cmd[96];
    snprintf(cmd, sizeof cmd, "AT+QFLST=\"%s\"", pat ? pat : "*");

    struct {
        RIL_FILE_ListCB cb;
        void* ctx;
    } t = {cb, ctx};

    return (RIL_SendATCmd(cmd, strlen(cmd), cb_QFLST, &t, 4000) == RIL_AT_SUCCESS)
               ? RIL_FILE_OK
               : RIL_FILE_ERR_TIMEOUT;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                      File & Directory Management                        */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Delete a file from modem storage
 * @param f Filename to delete
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFDEL command to delete the specified file.
 */
RIL_FILE_Err RIL_FILE_Delete(const char* f) {
    if (!f)
        return RIL_FILE_ERR_PARAM;

    char cmd[96];
    snprintf(cmd, sizeof cmd, "AT+QFDEL=\"%s\"", f);
    return AT_OK(cmd, 3000);
}

/**
 * @brief Create a directory on modem storage
 * @param d Directory name to create
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFMKDIR command to create the specified directory.
 */
RIL_FILE_Err RIL_FILE_Mkdir(const char* d) {
    if (!d)
        return RIL_FILE_ERR_PARAM;

    char cmd[96];
    snprintf(cmd, sizeof cmd, "AT+QFMKDIR=\"%s\"", d);
    return AT_OK(cmd, 4000);
}

/**
 * @brief Remove a directory from modem storage
 * @param d Directory name to remove
 * @param force If true, remove non-empty directory recursively
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFRMDIR command to remove the specified directory.
 *          The force parameter determines whether to remove non-empty directories.
 */
RIL_FILE_Err RIL_FILE_Rmdir(const char* d, bool force) {
    if (!d)
        return RIL_FILE_ERR_PARAM;

    char cmd[96];
    snprintf(cmd, sizeof cmd, "AT+QFRMDIR=\"%s\"%s", d, force ? ",1" : "");
    return AT_OK(cmd, 4000);
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                         File Upload Operations                          */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Callback for parsing QFUPL response (upload result)
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to integer for error code)
 * @return RIL_ATRSP_SUCCESS when QFUPL response is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QFUPL: upload_size,error_code" response
 */
static int32_t cb_QFUPL(char* line, uint32_t len, void* ud) {
    int* e = ud;
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    } else if (e != NULL && !strncmp(line, "+QFUPL:", 7)) {
        sscanf(line, "+QFUPL: %*u,%d", e);
        return RIL_ATRSP_CONTINUE;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Upload data to a file on modem storage
 * @param dst Destination filename on modem
 * @param data Pointer to data buffer to upload
 * @param len Length of data in bytes
 * @param tout Timeout for upload operation in seconds
 * @param ack If true, wait for acknowledgment after upload
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFUPL command followed by binary data transfer.
 *          The operation consists of two phases: command setup and data transfer.
 */
RIL_FILE_Err RIL_FILE_Upload(const char* dst, const uint8_t* data, uint32_t len, uint16_t tout,
                             bool ack) {
    if (!dst || !data || !len)
        return RIL_FILE_ERR_PARAM;

    /* Phase 1: Send upload command */
    char cmd[128];
    snprintf(cmd, sizeof cmd, "AT+QFUPL=\"%s\",%u,%u,%u", dst, len, tout, ack ? 1 : 0);
    if (RIL_SendATCmd(cmd, strlen(cmd), NULL, NULL, (tout + 5) * 1000) != RIL_AT_SUCCESS) {
        return map_err(RIL_AT_GetErrCode());
    }

    /* Phase 2: Send binary data and get result */
    if (RIL_SendBinaryData(data, len, cb_QFUPL, NULL, 30000) != RIL_AT_SUCCESS)
        return map_err(RIL_AT_GetErrCode());

    return RIL_FILE_OK;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                        File Download Operations                         */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Download a file from modem storage
 * @param src Source filename on modem
 * @param cb Callback function to receive downloaded data
 * @param userData User context passed to callback function
 * @param wait Maximum wait time in seconds
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFDWL command to download the specified file.
 *          Downloaded data is streamed to the user via callback function.
 */
RIL_FILE_Err RIL_FILE_Download(const char* src, RIL_FILE_DataCB cb, void* userData, uint16_t wait) {
    if (!src || !cb)
        return RIL_FILE_ERR_PARAM;

    char cmd[128];
    snprintf(cmd, sizeof cmd, "AT+QFDWL=\"%s\"", src);

    /* Set up streaming context */
    _BufCtx ctx = {
        .buffer = NULL,       /* Buffer is not used in streaming mode */
        .bufferSize = 0,      /* Buffer is not used in streaming mode */
        .bufferUsed = 0,      /* Buffer is not used in streaming mode */
        .dataCallback = cb,   /* User data callback */
        .userData = userData, /* User context */
    };

    if (RIL_SendATCmd(cmd, strlen(cmd), cb_copy, &ctx, (wait + 10) * 1000) != RIL_AT_SUCCESS)
        return map_err(RIL_AT_GetErrCode());

    return RIL_FILE_OK;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                         Random Access File API                         */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Callback for parsing QFOPEN response (file handle)
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to integer for file handle)
 * @return RIL_ATRSP_SUCCESS when QFOPEN response is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QFOPEN: file_handle" response
 */
static int32_t cb_QFOPEN(char* line, uint32_t len, void* ud) {
    int* h = ud;
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    } else if (!strncmp(line, "+QFOPEN:", 8)) {
        sscanf(line, "+QFOPEN: %d", h);
        return RIL_ATRSP_CONTINUE;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Open a file for read/write operations
 * @param p File path to open
 * @param m Open mode (RIL_FILE_MODE_CREATE_RW, RIL_FILE_MODE_CREATE_CLR,
 * RIL_FILE_MODE_READONLY)
 * @param out Pointer to store the file handle
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFOPEN command to open a file with specified mode.
 *          Returns a handle that can be used for subsequent file operations.
 */
RIL_FILE_Err RIL_FILE_Open(const char* p, RIL_FILE_Mode m, RIL_FILE_Handle* out) {
    if (!p || !out)
        return RIL_FILE_ERR_PARAM;

    char cmd[128];
    snprintf(cmd, sizeof cmd, "AT+QFOPEN=\"%s\",%u", p, m);

    int h = -1;
    if (RIL_SendATCmd(cmd, strlen(cmd), cb_QFOPEN, &h, 3000) != RIL_AT_SUCCESS)
        return map_err(RIL_AT_GetErrCode());

    if (h < 0)
        return map_err(h);

    *out = h;
    return RIL_FILE_OK;
}

/**
 * @brief Read data from an opened file
 * @param h File handle from RIL_FILE_Open()
 * @param buf Buffer to store read data
 * @param len Maximum number of bytes to read
 * @param act Pointer to store actual bytes read (can be NULL)
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFREAD command to read data from the file.
 *          The operation consists of sending the read command and receiving data.
 */
RIL_FILE_Err RIL_FILE_Read(RIL_FILE_Handle h, uint8_t* buf, uint32_t len, uint32_t* act) {
    if (!buf || !len)
        return RIL_FILE_ERR_PARAM;

    char cmd[32];
    snprintf(cmd, sizeof cmd, "AT+QFREAD=%d,%u", h, len);
    *act = 0;

    _BufCtx ctx = {
        .buffer = buf,        /* Buffer to store read data */
        .bufferSize = len,    /* Buffer capacity in bytes */
        .bufferUsed = 0,      /* Number of bytes currently used in buffer */
        .dataCallback = NULL, /* User data callback */
        .userData = act,      /* User context */
    };

    if (RIL_SendATCmd(cmd, strlen(cmd), cb_copy, &ctx, 30000) != RIL_AT_SUCCESS) {
        RIL_FILE_Err err = map_err(RIL_AT_GetErrCode());
        if (err == RIL_FILE_ERR_EOF) {
            return RIL_FILE_OK;
        } else {
            return err;
        }
    }

    return RIL_FILE_OK;
}

/**
 * @brief Callback for parsing QFWRITE response (write operation result)
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to integer for bytes written)
 * @return RIL_ATRSP_SUCCESS when QFWRITE response is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QFWRITE: error_code,bytes_written" response
 */
static int32_t cb_QFWRITE(char* line, uint32_t len, void* ud) {
    int* w = ud;
    int e;
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    } else if (!strncmp(line, "+QFWRITE:", 9)) {
        sscanf(line, "+QFWRITE: %d,%d", &e, w);
        return RIL_ATRSP_CONTINUE;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Write data to an opened file
 * @param h File handle from RIL_FILE_Open()
 * @param buf Buffer containing data to write
 * @param len Number of bytes to write
 * @param wrote Pointer to store actual bytes written (can be NULL)
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFWRITE command followed by binary data transfer.
 *          Returns error if not all bytes were written successfully.
 */
RIL_FILE_Err RIL_FILE_Write(RIL_FILE_Handle h, const uint8_t* buf, uint32_t len, uint32_t* wrote) {
    if (!buf || !len)
        return RIL_FILE_ERR_PARAM;

    /* Phase 1: Send write command */
    char cmd[32];
    snprintf(cmd, sizeof cmd, "AT+QFWRITE=%d,%u", h, len);
    if (RIL_SendATCmd(cmd, strlen(cmd), NULL, NULL, 5000) != RIL_AT_SUCCESS)
        return map_err(RIL_AT_GetErrCode());

    /* Phase 2: Send binary data and get result */
    int wr = 0;
    if (RIL_SendBinaryData(buf, len, cb_QFWRITE, &wr, 30000) != RIL_AT_SUCCESS)
        return map_err(RIL_AT_GetErrCode());

    if (wrote)
        *wrote = wr;

    return (wr == (int) len) ? RIL_FILE_OK : RIL_FILE_ERR_WRITE;
}

/**
 * @brief Seek to a position in an opened file
 * @param h File handle from RIL_FILE_Open()
 * @param off Offset value (positive or negative)
 * @param whence Seek origin (0=from start, 1=from current, 2=from end)
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFSEEK command to change the file position pointer.
 */
RIL_FILE_Err RIL_FILE_Seek(RIL_FILE_Handle h, int32_t off, uint8_t whence) {
    char cmd[48];
    snprintf(cmd, sizeof cmd, "AT+QFSEEK=%d,%d,%u", h, off, whence);
    return AT_OK(cmd, 3000);
}

/**
 * @brief Callback for parsing QFPOSITION response (current file position)
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to uint32_t for position)
 * @return RIL_ATRSP_SUCCESS when QFPOSITION response is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QFPOSITION: position" response
 */
static int32_t cb_QFPOS(char* line, uint32_t len, void* ud) {
    uint32_t* p = ud;
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    } else if (!strncmp(line, "+QFPOSITION:", 12)) {
        sscanf(line, "+QFPOSITION: %u", p);
        return RIL_ATRSP_CONTINUE;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Get current position in an opened file
 * @param h File handle from RIL_FILE_Open()
 * @param off Pointer to store current file position
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFPOSITION command to query current file position.
 */
RIL_FILE_Err RIL_FILE_Position(RIL_FILE_Handle h, uint32_t* off) {
    if (!off)
        return RIL_FILE_ERR_PARAM;

    char cmd[32];
    snprintf(cmd, sizeof cmd, "AT+QFPOSITION=%d", h);

    return (RIL_SendATCmd(cmd, strlen(cmd), cb_QFPOS, off, 3000) == RIL_AT_SUCCESS)
               ? RIL_FILE_OK
               : map_err(RIL_AT_GetErrCode());
}

/**
 * @brief Close an opened file
 * @param h File handle from RIL_FILE_Open()
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFCLOSE command to close the file and free resources.
 */
RIL_FILE_Err RIL_FILE_Close(RIL_FILE_Handle h) {
    char cmd[24];
    snprintf(cmd, sizeof cmd, "AT+QFCLOSE=%d", h);
    return AT_OK(cmd, 2500);
}

/**
 * @brief Truncate a file to specified length
 * @param path File path to truncate
 * @param nlen New length of file in bytes
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFTRUNC command to truncate the file to specified size.
 */
RIL_FILE_Err RIL_FILE_Trunc(const char* path, uint32_t nlen) {
    if (!path)
        return RIL_FILE_ERR_PARAM;

    char cmd[128];
    snprintf(cmd, sizeof cmd, "AT+QFTRUNC=\"%s\",%u", path, nlen);
    return AT_OK(cmd, 4000);
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                            Utility Functions                           */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Callback for parsing QFSIZE response (file size)
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to uint32_t for file size)
 * @return RIL_ATRSP_SUCCESS when QFSIZE response is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QFSIZE: size" response
 */
static int32_t cb_QFSIZE(char* line, uint32_t len, void* ud) {
    uint32_t* s = ud;
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    } else if (!strncmp(line, "+QFSIZE:", 8)) {
        sscanf(line, "+QFSIZE: %u", s);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Get size of a file
 * @param p File path to query
 * @param s Pointer to store file size in bytes
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFSIZE command to query the size of specified file.
 */
RIL_FILE_Err RIL_FILE_Size(const char* p, uint32_t* s) {
    if (!p || !s)
        return RIL_FILE_ERR_PARAM;

    char cmd[128];
    snprintf(cmd, sizeof cmd, "AT+QFSIZE=\"%s\"", p);

    return (RIL_SendATCmd(cmd, strlen(cmd), cb_QFSIZE, s, 3000) == RIL_AT_SUCCESS)
               ? RIL_FILE_OK
               : map_err(RIL_AT_GetErrCode());
}

/**
 * @brief Callback for parsing QFMEM response (free memory)
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to uint32_t for free memory)
 * @return RIL_ATRSP_SUCCESS when QFMEM response is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QFMEM: free_memory" response
 */
static int32_t cb_QFMEM(char* line, uint32_t len, void* ud) {
    uint32_t* f = ud;
    if (strncmp(line, "OK", len) == 0) {
        return RIL_ATRSP_SUCCESS;
    } else if (!strncmp(line, "+QFMEM:", 7)) {
        sscanf(line, "+QFMEM: %u", f);
        return RIL_ATRSP_CONTINUE;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Get free memory information
 * @param freeB Pointer to store free memory in bytes
 * @return RIL_FILE_OK on success, error code on failure
 * @details Executes AT+QFMEM command to query available free memory.
 */
RIL_FILE_Err RIL_FILE_GetFree(uint32_t* freeB) {
    if (!freeB)
        return RIL_FILE_ERR_PARAM;

    return (RIL_SendATCmd("AT+QFMEM", 7, cb_QFMEM, freeB, 3000) == RIL_AT_SUCCESS)
               ? RIL_FILE_OK
               : map_err(RIL_AT_GetErrCode());
}
