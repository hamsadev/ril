/**
 * @file   ril_file.h
 * @brief  High-level wrapper around Quectel FILE AT commands
 *         Provides a comprehensive interface for file system operations
 *         including file manipulation, directory operations, and data transfer.
 *
 * @details This module wraps Quectel modem file system commands:
 *          - AT+QFLDS: Get storage space information
 *          - AT+QFLST: List files and directories
 *          - AT+QFDEL: Delete files
 *          - AT+QFUPL: Upload files to modem storage
 *          - AT+QFDWL: Download files from modem storage
 *          - AT+QFOPEN: Open files for read/write operations
 *          - AT+QFREAD: Read data from opened files
 *          - AT+QFWRITE: Write data to opened files
 *          - AT+QFSEEK: Seek to position in file
 *          - AT+QFPOSITION: Get current file position
 *          - AT+QFCLOSE: Close opened files
 *          - AT+QFMKDIR: Create directories
 *          - AT+QFRMDIR: Remove directories
 *          - AT+QFTRUNC: Truncate files
 *          - AT+QFSIZE: Get file size
 *          - AT+QFMEM: Get free memory information
 *
 * @dependencies ril.h (RIL_SendATCmd / RIL_SendBinaryData)
 * @note All APIs are blocking and return RIL_FILE_Err status codes.
 * @author Sepahtan Project Team
 * @version 1.0
 */

#ifndef RIL_FILE_H
#define RIL_FILE_H

#include "ril.h"
#include <stdbool.h>
#include <stdint.h>

#define RIL_FILE_LOG_ENABLE 1
#define RIL_FILE_LOG_VERBOSE 1

#if RIL_FILE_LOG_ENABLE
#define RIL_FILE_LOG_WARN(fmt, ...) logWarn(fmt, ##__VA_ARGS__)
#define RIL_FILE_LOG_ERROR(fmt, ...) logError(fmt, ##__VA_ARGS__)
#define RIL_FILE_LOG_DEBUG(fmt, ...) logDebug(fmt, ##__VA_ARGS__)
#define RIL_FILE_LOG_INFO(fmt, ...) logInfo(fmt, ##__VA_ARGS__)
#define RIL_FILE_LOG_TRACE(fmt, ...) logTrace(fmt, ##__VA_ARGS__)
#else
#define RIL_FILE_LOG_WARN(fmt, ...)
#define RIL_FILE_LOG_ERROR(fmt, ...)
#define RIL_FILE_LOG_DEBUG(fmt, ...)
#define RIL_FILE_LOG_INFO(fmt, ...)
#define RIL_FILE_LOG_TRACE(fmt, ...)
#endif

/**
 * @brief File operation error codes
 * @details These error codes correspond to Quectel modem file system errors
 *          as defined in the modem documentation Table-2.
 */
typedef enum {
    RIL_FILE_OK = 0,             /**< Operation successful */
    RIL_FILE_ERR_INVVAL = 400,   /**< Invalid parameter value */
    RIL_FILE_ERR_RANGE = 401,    /**< Parameter out of range */
    RIL_FILE_ERR_EOF = 402,      /**< End of file reached */
    RIL_FILE_ERR_FULL = 403,     /**< Storage full */
    RIL_FILE_ERR_NOTFOUND = 405, /**< File not found */
    RIL_FILE_ERR_BADNAME = 406,  /**< Invalid filename */
    RIL_FILE_ERR_EXIST = 407,    /**< File already exists */
    RIL_FILE_ERR_WRITE = 409,    /**< Write operation failed */
    RIL_FILE_ERR_OPEN = 410,     /**< Failed to open file */
    RIL_FILE_ERR_READ = 411,     /**< Read operation failed */
    RIL_FILE_ERR_MAXOPEN = 413,  /**< Maximum open files exceeded */
    RIL_FILE_ERR_RO = 414,       /**< Read-only file system */
    RIL_FILE_ERR_FSIZE = 415,    /**< File size error */
    RIL_FILE_ERR_FDESCR = 416,   /**< File descriptor error */
    RIL_FILE_ERR_LIST = 417,     /**< List operation failed */
    RIL_FILE_ERR_DEL = 418,      /**< Delete operation failed */
    RIL_FILE_ERR_NOMEM = 420,    /**< No memory available */
    RIL_FILE_ERR_TIMEOUT = 421,  /**< Operation timeout */
    RIL_FILE_ERR_TOOLARGE = 423, /**< File too large */
    RIL_FILE_ERR_PARAM = 425,    /**< Parameter error */
    RIL_FILE_ERR_BUSY = 426,     /**< System busy */
    RIL_FILE_ERR_UNKNOWN = 700   /**< Unknown error */
} RIL_FILE_Err;

typedef enum {
    /**< If the file does not exist, it will be created. If the file exists, it will be
              directly opened. And both of them can be read and written. */
    RIL_FILE_MODE_CREATE_RW = 0,
    /**< If the file does not exist, it will be created. If the file exists, it will be
          overwritten and cleared. And both of them can be read and written.  */
    RIL_FILE_MODE_CREATE_CLR = 1,
    /**< If the file exists, open it and it can be read only. When the
                               file does not exist, it will respond an error */
    RIL_FILE_MODE_READONLY = 2,
} RIL_FILE_Mode;

/**
 * @brief File handle type
 * @details Handle returned by RIL_FILE_Open() for file operations
 */
typedef int32_t RIL_FILE_Handle;

/**
 * @brief Callback function for file listing operations
 * @param name File or directory name
 * @param size Size of the file in bytes (0 for directories)
 * @param ctx User-provided context pointer
 */
typedef void (*RIL_FILE_ListCB)(const char* name, uint32_t size, void* ctx);

/**
 * @brief Callback function for data streaming operations
 * @param blk Pointer to data block
 * @param len Length of data block in bytes
 * @param ctx User-provided context pointer
 */
typedef void (*RIL_FILE_DataCB)(const uint8_t* blk, uint32_t len, void* ctx);

/* ────────────────────────────────────────────────────────────────────── */
/*                              PUBLIC API                                 */
/* ────────────────────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Storage Information & Listing                    */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Get storage space information
 * @param medium Storage medium name (e.g., "UFS", "RAM"). If NULL, defaults to "UFS"
 * @param freeB Pointer to store free space in bytes
 * @param totalB Pointer to store total space in bytes
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFLDS command
 */
RIL_FILE_Err RIL_FILE_GetSpace(const char* medium, uint32_t* freeB, uint32_t* totalB);

/**
 * @brief List files and directories matching a pattern
 * @param pattern Search pattern (e.g., "*.txt", "*"). If NULL, defaults to "*"
 * @param cb Callback function called for each matching item
 * @param ctx User context passed to callback function
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFLST command
 */
RIL_FILE_Err RIL_FILE_List(const char* pattern, RIL_FILE_ListCB cb, void* ctx);

/* ══════════════════════════════════════════════════════════════════════ */
/*                        File & Directory Operations                      */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Delete a file
 * @param filename Name of file to delete
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFDEL command
 */
RIL_FILE_Err RIL_FILE_Delete(const char* filename);

/**
 * @brief Create a directory
 * @param dirName Name of directory to create
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFMKDIR command
 */
RIL_FILE_Err RIL_FILE_Mkdir(const char* dirName);

/**
 * @brief Remove a directory
 * @param dirName Name of directory to remove
 * @param force If true, remove non-empty directory recursively
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFRMDIR command
 */
RIL_FILE_Err RIL_FILE_Rmdir(const char* dirName, bool force);

/* ══════════════════════════════════════════════════════════════════════ */
/*                        File Transfer Operations                         */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Upload data to a file on modem storage
 * @param destName Destination filename on modem
 * @param data Pointer to data buffer to upload
 * @param len Length of data in bytes
 * @param timeoutSec Timeout for upload operation in seconds
 * @param ackMode If true, wait for acknowledgment after upload
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFUPL command with binary data transfer
 */
RIL_FILE_Err RIL_FILE_Upload(const char* destName, const uint8_t* data, uint32_t len,
                             uint16_t timeoutSec, bool ackMode);

/**
 * @brief Download a file from modem storage
 * @param srcName Source filename on modem
 * @param cb Callback function to receive downloaded data
 * @param ctx User context passed to callback function
 * @param waitTimeSec Maximum wait time in seconds
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFDWL command, data is streamed via callback
 */
RIL_FILE_Err RIL_FILE_Download(const char* srcName, RIL_FILE_DataCB cb, void* ctx,
                               uint16_t waitTimeSec);

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Random Access File API                          */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Open a file for read/write operations
 * @param path File path to open
 * @param mode Open mode (RIL_FILE_MODE_CREATE_RW, RIL_FILE_MODE_CREATE_CLR,
 * RIL_FILE_MODE_READONLY)
 * @param out Pointer to store the file handle
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFOPEN command
 */
RIL_FILE_Err RIL_FILE_Open(const char* path, RIL_FILE_Mode mode, RIL_FILE_Handle* out);

/**
 * @brief Read data from an opened file
 * @param h File handle from RIL_FILE_Open()
 * @param buf Buffer to store read data
 * @param len Maximum number of bytes to read
 * @param act Pointer to store actual bytes read (can be NULL)
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFREAD command
 */
RIL_FILE_Err RIL_FILE_Read(RIL_FILE_Handle h, uint8_t* buf, uint32_t len, uint32_t* act);

/**
 * @brief Write data to an opened file
 * @param h File handle from RIL_FILE_Open()
 * @param buf Buffer containing data to write
 * @param len Number of bytes to write
 * @param wrote Pointer to store actual bytes written (can be NULL)
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFWRITE command
 */
RIL_FILE_Err RIL_FILE_Write(RIL_FILE_Handle h, const uint8_t* buf, uint32_t len, uint32_t* wrote);

/**
 * @brief Seek to a position in an opened file
 * @param h File handle from RIL_FILE_Open()
 * @param offset Offset value (positive or negative)
 * @param whence Seek origin (0=from start, 1=from current, 2=from end)
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFSEEK command
 */
RIL_FILE_Err RIL_FILE_Seek(RIL_FILE_Handle h, int32_t offset, uint8_t whence);

/**
 * @brief Get current position in an opened file
 * @param h File handle from RIL_FILE_Open()
 * @param offset Pointer to store current file position
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFPOSITION command
 */
RIL_FILE_Err RIL_FILE_Position(RIL_FILE_Handle h, uint32_t* offset);

/**
 * @brief Close an opened file
 * @param h File handle from RIL_FILE_Open()
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFCLOSE command
 */
RIL_FILE_Err RIL_FILE_Close(RIL_FILE_Handle h);

/**
 * @brief Truncate a file to specified length
 * @param path File path to truncate
 * @param newLen New length of file in bytes
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFTRUNC command
 */
RIL_FILE_Err RIL_FILE_Trunc(const char* path, uint32_t newLen);

/* ══════════════════════════════════════════════════════════════════════ */
/*                            Utility Functions                           */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Get size of a file
 * @param path File path to query
 * @param size Pointer to store file size in bytes
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFSIZE command
 */
RIL_FILE_Err RIL_FILE_Size(const char* path, uint32_t* size);

/**
 * @brief Get free memory information
 * @param bytesFree Pointer to store free memory in bytes
 * @return RIL_FILE_OK on success, error code on failure
 * @note Executes AT+QFMEM command
 */
RIL_FILE_Err RIL_FILE_GetFree(uint32_t* bytesFree);

#ifdef __cplusplus
}
#endif

#endif /* RIL_FILE_H */
