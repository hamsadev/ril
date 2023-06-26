/**
 * @file   ril_http.c
 * @brief  Implementation of HTTP/HTTPS client for Quectel EC200/EG915U modems
 * @details Provides implementation for HTTP client operations using Quectel
 *          modem AT commands. This module covers the following operations:
 *          - HTTP configuration (QHTTPCFG family)
 *          - URL management (QHTTPURL)
 *          - HTTP request methods (QHTTPGET, QHTTPPOST, QHTTPPUT)
 *          - Response handling (QHTTPREAD, QHTTPREADFILE)
 *          - Connection management (QHTTPSTOP)
 *
 * @author Sepahtan Project Team
 * @version 1.0
 * @date 2024
 */
#include "ril_http.h"
#include <stdio.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════ */
/*                              Helper Functions                           */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Map Quectel error codes to RIL_HTTP_Err enum values
 * @param e Integer error code from modem response
 * @return Corresponding RIL_HTTP_Err enum value
 * @details Converts modem-specific error codes to standardized enum values.
 *          Error codes 701-733 are mapped directly, other values return unknown error.
 */
static inline RIL_HTTP_Err map_err(int e) {
    if (e == 0)
        return RIL_HTTP_OK;
    if (e >= 701 && e <= 733)
        return (RIL_HTTP_Err) e;
    return RIL_HTTP_ERR_UNKNOWN;
}

/**
 * @brief Helper function for boolean configuration commands
 * @param fmt Format string for AT command with %u placeholder for boolean value
 * @param en Boolean value to set (true = 1, false = 0)
 * @return RIL_HTTP_OK on success, RIL_HTTP_ERR_TIMEOUT on failure
 */
static RIL_HTTP_Err cfg_bool(const char* fmt, bool en) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), fmt, en ? 1 : 0);
    return (RIL_SendATCmd(cmd, strlen(cmd), NULL, NULL, 10000) == RIL_AT_SUCCESS)
               ? RIL_HTTP_OK
               : RIL_HTTP_ERR_TIMEOUT;
}

/**
 * @brief Helper function for 8-bit integer configuration commands
 * @param fmt Format string for AT command with %u placeholder for uint8_t value
 * @param v 8-bit unsigned integer value to set
 * @return RIL_HTTP_OK on success, RIL_HTTP_ERR_TIMEOUT on failure
 */
static RIL_HTTP_Err cfg_u8(const char* fmt, uint8_t v) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), fmt, v);
    return (RIL_SendATCmd(cmd, strlen(cmd), NULL, NULL, 10000) == RIL_AT_SUCCESS)
               ? RIL_HTTP_OK
               : RIL_HTTP_ERR_TIMEOUT;
}

/**
 * @brief Helper function for string configuration commands
 * @param fmt Format string for AT command with %s placeholder for string value
 * @param s String value to set (will be quoted in command)
 * @return RIL_HTTP_OK on success, RIL_HTTP_ERR_TIMEOUT on failure
 */
static RIL_HTTP_Err cfg_str(const char* fmt, const char* s) {
    char cmd[192];
    snprintf(cmd, sizeof(cmd), fmt, s);
    return (RIL_SendATCmd(cmd, strlen(cmd), NULL, NULL, 20000) == RIL_AT_SUCCESS)
               ? RIL_HTTP_OK
               : RIL_HTTP_ERR_TIMEOUT;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Initialization Functions                        */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialize HTTP client context with default values
 * @param cli Pointer to client context structure
 * @param cid PDP context ID (1-7)
 * @param sslctx SSL context ID (0 for HTTP, 1-5 for HTTPS)
 * @details Zeros out the entire structure and sets the specified context IDs.
 *          All other fields are initialized to safe default values.
 */
void RIL_HTTP_Init(RIL_HTTPClient* cli, uint8_t cid, uint8_t sslctx) {
    if (!cli)
        return;
    memset(cli, 0, sizeof(*cli));
    cli->cid = cid;
    cli->sslctx = sslctx;
    cli->lastErr = RIL_HTTP_OK;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Configuration Functions                         */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Configure PDP context ID for HTTP session
 * @param cli Pointer to client context
 * @param cid PDP context ID (1-7)
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Updates the client context and sends AT+QHTTPCFG="contextid",<cid>
 */
RIL_HTTP_Err RIL_HTTP_CfgContextId(RIL_HTTPClient* cli, uint8_t cid) {
    if (!cli)
        return RIL_HTTP_ERR_ARG;
    cli->cid = cid;
    return cfg_u8("AT+QHTTPCFG=\"contextid\",%u", cid);
}

/**
 * @brief Configure SSL context for HTTPS connections
 * @param cli Pointer to client context
 * @param sslctx SSL context ID (0=HTTP, 1-5=HTTPS)
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Updates the client context and sends AT+QHTTPCFG="sslctxid",<cid>,<sslctx>
 */
RIL_HTTP_Err RIL_HTTP_CfgSSL(RIL_HTTPClient* cli, uint8_t sslctx) {
    if (!cli)
        return RIL_HTTP_ERR_ARG;
    cli->sslctx = sslctx;
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+QHTTPCFG=\"sslctxid\",%u,%u", cli->cid, sslctx);
    return (RIL_SendATCmd(cmd, strlen(cmd), NULL, NULL, 10000) == RIL_AT_SUCCESS)
               ? RIL_HTTP_OK
               : RIL_HTTP_ERR_TIMEOUT;
}

/**
 * @brief Enable/disable custom request headers
 * @param cli Pointer to client context
 * @param enable If true, enable sending custom request headers from MCU
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Updates the client context and sends AT+QHTTPCFG="requestheader",<0|1>
 */
RIL_HTTP_Err RIL_HTTP_CfgReqHeader(RIL_HTTPClient* cli, bool enable) {
    if (!cli)
        return RIL_HTTP_ERR_ARG;
    cli->custHdr = enable;
    return cfg_bool("AT+QHTTPCFG=\"requestheader\",%u", enable);
}

/**
 * @brief Enable/disable response header inclusion
 * @param cli Pointer to client context
 * @param enable If true, include HTTP response headers when reading
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Updates the client context and sends AT+QHTTPCFG="responseheader",<0|1>
 */
RIL_HTTP_Err RIL_HTTP_CfgRspHeader(RIL_HTTPClient* cli, bool enable) {
    if (!cli)
        return RIL_HTTP_ERR_ARG;
    cli->respHdr = enable;
    return cfg_bool("AT+QHTTPCFG=\"responseheader\",%u", enable);
}

/**
 * @brief Configure Content-Type for request body
 * @param cli Pointer to client context
 * @param mime MIME type string (e.g., "application/json", "text/plain")
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Sends AT+QHTTPCFG="contenttype","<mime>"
 */
RIL_HTTP_Err RIL_HTTP_CfgContentType(RIL_HTTPClient* cli, const char* mime) {
    if (!cli || !mime)
        return RIL_HTTP_ERR_ARG;
    return cfg_str("AT+QHTTPCFG=\"contenttype\",\"%s\"", mime);
}

/**
 * @brief Configure User-Agent header
 * @param cli Pointer to client context
 * @param ua User-Agent string
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Sends AT+QHTTPCFG="useragent","<string>"
 */
RIL_HTTP_Err RIL_HTTP_CfgUserAgent(RIL_HTTPClient* cli, const char* ua) {
    if (!cli || !ua)
        return RIL_HTTP_ERR_ARG;
    return cfg_str("AT+QHTTPCFG=\"useragent\",\"%s\"", ua);
}

/**
 * @brief Configure HTTP Basic Authentication
 * @param cli Pointer to client context
 * @param user Username for authentication
 * @param pass Password for authentication
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Sends AT+QHTTPCFG="auth","user:pass"
 */
RIL_HTTP_Err RIL_HTTP_CfgAuthBasic(RIL_HTTPClient* cli, const char* user, const char* pass) {
    if (!cli || !user || !pass)
        return RIL_HTTP_ERR_ARG;
    char cmd[192];
    snprintf(cmd, sizeof(cmd), "AT+QHTTPCFG=\"auth\",\"%s:%s\"", user, pass);
    return (RIL_SendATCmd(cmd, strlen(cmd), NULL, NULL, 10000) == RIL_AT_SUCCESS)
               ? RIL_HTTP_OK
               : RIL_HTTP_ERR_TIMEOUT;
}

/**
 * @brief Enable/disable multipart form data support
 * @param cli Pointer to client context
 * @param enable If true, enable multipart/form-data for POSTFILE/PUTFILE
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Sends AT+QHTTPCFG="formdata",<0|1>
 */
RIL_HTTP_Err RIL_HTTP_CfgFormData(RIL_HTTPClient* cli, bool enable) {
    if (!cli)
        return RIL_HTTP_ERR_ARG;
    return cfg_bool("AT+QHTTPCFG=\"formdata\",%u", enable);
}

/**
 * @brief Enable/disable connection close notifications
 * @param cli Pointer to client context
 * @param enable If true, request URC notification when HTTP connection closes
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Sends AT+QHTTPCFG="closedind",<0|1>
 */
RIL_HTTP_Err RIL_HTTP_CfgClosedInd(RIL_HTTPClient* cli, bool enable) {
    if (!cli)
        return RIL_HTTP_ERR_ARG;
    return cfg_bool("AT+QHTTPCFG=\"closedind\",%u", enable);
}

/**
 * @brief Reset all HTTP configuration to defaults
 * @param cli Pointer to client context (not used but kept for API consistency)
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Sends AT+QHTTPCFG="del" to restore factory defaults
 */
RIL_HTTP_Err RIL_HTTP_ResetCfg(RIL_HTTPClient* cli) {
    (void) cli;
    return (RIL_SendATCmd("AT+QHTTPCFG=\"del\"", 15, NULL, NULL, 10000) == RIL_AT_SUCCESS)
               ? RIL_HTTP_OK
               : RIL_HTTP_ERR_TIMEOUT;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                            URL Management                              */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Callback for parsing QHTTPURL command responses
 * @param line Response line from modem
 * @param len Length of response line
 * @param userData User data (not used)
 * @return RIL_ATRSP_SUCCESS for CONNECT/OK responses, RIL_ATRSP_CONTINUE otherwise
 * @details Handles CONNECT and OK responses during URL upload process
 */
int32_t AT_QHTTPURL(char* line, uint32_t len, void* userData) {
    if (strncmp(line, "CONNECT", 7) == 0 || strncmp(line, "OK", 2) == 0) {
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Upload URL to the modem for HTTP operations
 * @param cli Pointer to client context
 * @param url Zero-terminated URL string (max 65535 bytes)
 * @param timeoutSec Timeout for URL upload operation in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Performs a two-phase URL upload:
 *          Phase 1: Send AT+QHTTPURL=<len>,<timeout> and wait for CONNECT
 *          Phase 2: Send URL data and wait for final OK
 */
RIL_HTTP_Err RIL_HTTP_SetURL(RIL_HTTPClient* cli, const char* url, uint16_t timeoutSec) {
    if (!cli || !url)
        return RIL_HTTP_ERR_ARG;

    size_t len = strlen(url);
    if (len == 0 || len > 65535)
        return RIL_HTTP_ERR_ARG;

    RIL_LOG_TRACE("RIL_HTTP_SetURL, url: %s, len: %zu, timeoutSec: %u", url, len, timeoutSec);

    /* Phase 1: Announce URL length and timeout, wait for CONNECT */
    char cmd[48];
    snprintf(cmd, sizeof(cmd), "AT+QHTTPURL=%zu,%u", len, timeoutSec);

    if (RIL_SendATCmd(cmd, strlen(cmd), AT_QHTTPURL, NULL, (timeoutSec + 5) * 1000) !=
        RIL_AT_SUCCESS)
        return RIL_HTTP_ERR_TIMEOUT;

    /* Phase 2: Send URL data and wait for final OK */
    if (RIL_SendBinaryData((const uint8_t*) url, (uint32_t) len, AT_QHTTPURL, NULL, 20000) !=
        RIL_AT_SUCCESS)
        return RIL_HTTP_ERR_TIMEOUT;

    cli->lastErr = RIL_HTTP_OK;
    return cli->lastErr;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                          HTTP Request Methods                          */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Structure for storing HTTP transaction results
 * @details Used by callbacks to capture response information from URC messages
 */
typedef struct {
    int err;      /**< Error code (0 or 701-733) */
    int code;     /**< HTTP status code (e.g., 200, 404) */
    uint32_t len; /**< Content-Length if provided by module */
} TrxRes;

/**
 * @brief Callback for parsing QHTTPGET URC responses
 * @param line Response line from modem
 * @param len Length of response line
 * @param userData Pointer to TrxRes structure
 * @return RIL_ATRSP_SUCCESS when URC is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QHTTPGET: <err>,<code>,<len>" URC messages
 */
int32_t AT_QHTTPGET(char* line, uint32_t len, void* userData) {
    TrxRes* r = (TrxRes*) userData;

    if (!strncmp(line, "+QHTTPGET:", 10)) {
        sscanf(line, "+QHTTPGET: %d,%d,%u", &r->err, &r->code, &r->len);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Finish HTTP transaction by updating client context
 * @param cli Pointer to client context
 * @param r Pointer to transaction result structure
 * @return Mapped error code from transaction result
 * @details Updates client's lastErr, lastStatus, and lastLength fields
 */
static RIL_HTTP_Err finish_trx(RIL_HTTPClient* cli, TrxRes* r) {
    cli->lastErr = map_err(r->err);
    cli->lastStatus = r->code;
    cli->lastLength = r->len;
    return cli->lastErr;
}

/**
 * @brief Perform HTTP GET request
 * @param cli Pointer to client context
 * @param rspTimeSec Timeout for response in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Sends AT+QHTTPGET=<timeout> and waits for URC response.
 *          Includes additional margin time for network operations.
 */
RIL_HTTP_Err RIL_HTTP_Get(RIL_HTTPClient* cli, uint16_t rspTimeSec) {
    if (!cli)
        return RIL_HTTP_ERR_ARG;

    TrxRes res = {-1, -1, 0};
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QHTTPGET=%u", rspTimeSec);

    /* Add margin time: rspTimeSec + 10 seconds */
    uint32_t tout = (rspTimeSec + 10) * 1000;

    /* Send command and wait for URC response */
    if (RIL_SendATCmd(cmd, strlen(cmd), AT_QHTTPGET, &res, tout) != RIL_AT_SUCCESS)
        return RIL_HTTP_ERR_TIMEOUT;

    return finish_trx(cli, &res);
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                       Response Reading Functions                       */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Context structure for HTTP response reading operations
 * @details Used by response reading callbacks to manage data buffering and streaming
 */
typedef struct {
    uint8_t* buf;        /**< Destination buffer for data */
    uint32_t cap;        /**< Buffer capacity in bytes */
    uint32_t used;       /**< Bytes currently stored in buffer */
    RIL_HTTP_ChunkCB cb; /**< User callback for streaming (NULL for buffering) */
    void* usr;           /**< User context for callback */
    int err;             /**< Error code from response parsing */
} HttpReadCtx;

/**
 * @brief Universal callback for handling HTTP response data
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to HttpReadCtx structure)
 * @return RIL_ATRSP_SUCCESS when end URC is found, RIL_ATRSP_CONTINUE otherwise
 * @details Handles both QHTTPREAD and QFREAD data streams.
 *          Supports both buffering and streaming modes based on context configuration.
 */
static int32_t cb_copyData(char* line, uint32_t len, void* ud) {
    HttpReadCtx* c = ud;

    /* Check for end-of-stream URC messages */
    if (!strncmp(line, "+QHTTPREAD:", 11) || !strncmp(line, "+QFREAD:", 8)) {
        sscanf(line, "+%*[^:]: %d", &c->err);
        if (c->cb && c->used) /* Flush remaining data in streaming mode */
            c->cb(c->buf, c->used, c->usr);
        return RIL_ATRSP_SUCCESS;
    }

    /* Skip control tokens */
    if (!strcmp(line, "CONNECT") || !strcmp(line, "OK"))
        return RIL_ATRSP_CONTINUE;

    /* Process payload data */
    const uint8_t* src = (const uint8_t*) line;
    while (len) {
        uint32_t space = c->cap - c->used;
        uint32_t take = (len < space) ? len : space;

        if (take)
            memcpy(c->buf + c->used, src, take);
        c->used += take;
        src += take;
        len -= take;

        /* Emit chunk when buffer is full (streaming mode only) */
        if (c->cb && c->used == c->cap) {
            c->cb(c->buf, c->cap, c->usr);
            c->used = 0;
        }
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Stream response data line-by-line
 * @param cli Pointer to client context
 * @param cb Callback function invoked for each line
 * @param ctx User context passed to callback
 * @param waitSec Timeout for read operation in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Uses a scratch buffer sized to match internal RIL line buffer (256 bytes).
 *          Suitable for text responses with regular line breaks.
 */
RIL_HTTP_Err RIL_HTTP_ReadLineStream(RIL_HTTPClient* cli, RIL_HTTP_ChunkCB cb, void* ctx,
                                     uint16_t waitSec) {
    if (!cli || !cb)
        return RIL_HTTP_ERR_ARG;

    /* Use scratch buffer matching internal RIL line buffer size */
    uint8_t scratch[256];
    HttpReadCtx hc = {scratch, sizeof scratch, 0, cb, ctx, -1};

    char cmd[32];
    snprintf(cmd, sizeof cmd, "AT+QHTTPREAD=%u", waitSec);

    /* Calculate timeout with content-based estimation */
    uint32_t tout = (waitSec + cli->lastLength / 50 + 30) * 1000;

    if (RIL_SendATCmd(cmd, strlen(cmd), cb_copyData, &hc, tout) != RIL_AT_SUCCESS)
        return RIL_HTTP_ERR_TIMEOUT;

    cli->lastErr = map_err(hc.err);
    return cli->lastErr;
}

/**
 * @brief Read full response into user-provided buffer
 * @param cli Pointer to client context
 * @param buf User buffer for response data
 * @param bufLen Capacity of user buffer in bytes
 * @param actual Pointer to store actual bytes read (can be NULL)
 * @param waitSec Timeout for read operation in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Copies the entire response into the provided buffer.
 *          If response is larger than buffer, only bufLen bytes are copied.
 */
RIL_HTTP_Err RIL_HTTP_ReadToBuf(RIL_HTTPClient* cli, uint8_t* buf, uint32_t bufLen,
                                uint32_t* actual, uint16_t waitSec) {
    if (!cli || !buf || !bufLen)
        return RIL_HTTP_ERR_ARG;

    HttpReadCtx hc = {buf, bufLen, 0, NULL, NULL, -1};

    char cmd[32];
    snprintf(cmd, sizeof cmd, "AT+QHTTPREAD=%u", waitSec);

    /* Calculate timeout with content-based estimation */
    uint32_t tout = (waitSec + cli->lastLength / 50 + 30) * 1000;

    if (RIL_SendATCmd(cmd, strlen(cmd), cb_copyData, &hc, tout) != RIL_AT_SUCCESS)
        return RIL_HTTP_ERR_TIMEOUT;

    if (actual)
        *actual = hc.used;

    cli->lastErr = map_err(hc.err);
    return cli->lastErr;
}

/**
 * @brief Callback for parsing QHTTPREADFILE responses
 * @param line Response line from modem
 * @param len Length of response line
 * @param ud User data (pointer to integer for error code)
 * @return RIL_ATRSP_SUCCESS when URC is found, RIL_ATRSP_CONTINUE otherwise
 * @details Parses "+QHTTPREADFILE: <err>" URC messages
 */
static int32_t cb_QHTTPREADFILE(char* line, uint32_t len, void* ud) {
    int* e = ud;
    if (!strncmp(line, "+QHTTPREADFILE:", 15)) {
        sscanf(line, "+QHTTPREADFILE: %d", e);
        return RIL_ATRSP_SUCCESS;
    }
    return RIL_ATRSP_CONTINUE;
}

/**
 * @brief Save response body directly to modem file system
 * @param cli Pointer to client context
 * @param fileName Destination filename on modem storage
 * @param waitSec Timeout for operation in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Executes AT+QHTTPREADFILE to save response directly to a file
 *          on the modem's internal file system.
 */
RIL_HTTP_Err RIL_HTTP_ReadFile(RIL_HTTPClient* cli, const char* fileName, uint16_t waitSec) {
    if (!cli || !fileName)
        return RIL_HTTP_ERR_ARG;

    int err = -1;
    char cmd[160];
    snprintf(cmd, sizeof cmd, "AT+QHTTPREADFILE=\"%s\",%u", fileName, waitSec);

    uint32_t tout = (waitSec + 10) * 1000;

    if (RIL_SendATCmd(cmd, strlen(cmd), cb_QHTTPREADFILE, &err, tout) != RIL_AT_SUCCESS)
        return RIL_HTTP_ERR_TIMEOUT;

    cli->lastErr = map_err(err);
    return cli->lastErr;
}

/**
 * @brief Stream previously saved file in chunks
 * @param cli Pointer to client context
 * @param fileName Name of file on modem storage to read
 * 1. The file name indicates the storage location. When the file name begins with "UFS:", it means
 * that the file is stored in UFS. When the file name begins with "SD:", it means that the file is
 * stored in SD card. And if there are no prefix characters in the file name, then the file is also
 * stored in UFS.
 * 2. Currently EC200U series and EG915U series modules do not support the storage medium "RAM:"
 * Random Access Memory.
 * 3. Currently EG915U series modules do not support the storage mediums "SD:" SD Card and "EFS:"
 * External Flash File System.
 * @param chunkSize Number of bytes per callback invocation
 * @param cb Callback function for data chunks
 * @param userCtx User context passed to callback
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Reads a file saved by RIL_HTTP_ReadFile() chunk by chunk using
 *          the RIL_FILE module for efficient streaming. The file is opened
 *          in read-only mode and read in the specified chunk size.
 */
RIL_HTTP_Err RIL_HTTP_ReadFileStream(RIL_HTTPClient* cli, const char* fileName, uint32_t chunkSize,
                                     RIL_HTTP_ChunkCB cb, void* userCtx) {
    if (!cli || !fileName || !cb || !chunkSize)
        return RIL_HTTP_ERR_ARG;

    RIL_FILE_Handle fh;

    /* Phase 1: Open file in read-only mode */
    if (RIL_FILE_Open(fileName, RIL_FILE_MODE_READONLY, &fh) != RIL_FILE_OK)
        return RIL_HTTP_ERR; /* Could extend error mapping if needed */

    uint8_t scratch[chunkSize];

    /* Phase 2: Read file in chunks and stream to user */
    while (1) {
        uint32_t got = 0;
        RIL_FILE_Err fe = RIL_FILE_Read(fh, scratch, chunkSize, &got);

        if (fe == RIL_FILE_OK && got) {
            if (!cb(scratch, got, userCtx)) {
                cli->lastErr = RIL_HTTP_ERR_CHUNK_FAILED;
                break;
            }
        }

        /* Check for end of file or error */
        if (fe == RIL_FILE_ERR_EOF || got == 0)
            break; /* End of file reached */

        if (fe != RIL_FILE_OK && fe != RIL_FILE_ERR_EOF) {
            cli->lastErr = RIL_HTTP_ERR;
            break;
        }
    }

    /* Phase 3: Clean up resources */
    RIL_FILE_Err fe = RIL_FILE_Close(fh);
    if (fe != RIL_FILE_OK) {
        cli->lastErr = RIL_HTTP_ERR;
        return cli->lastErr;
    }

    if (cli->lastErr == RIL_HTTP_ERR || cli->lastErr == RIL_HTTP_ERR_CHUNK_FAILED)
        return cli->lastErr;

    cli->lastErr = RIL_HTTP_OK;
    return cli->lastErr;
}
