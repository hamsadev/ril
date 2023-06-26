/**
 * @file   ril_http.h
 * @brief  Blocking HTTP/HTTPS client for Quectel EC200/EG915U modems
 * @details Provides a comprehensive HTTP client interface for Quectel modems
 *          supporting HTTP/HTTPS operations with SSL, authentication, and
 *          various data transfer modes including streaming and file operations.
 *
 * @details This module implements the complete Quectel HTTP client functionality
 *          as described in Application Note v1.1, covering:
 *          - Configuration commands (QHTTPCFG)
 *          - URL management (QHTTPURL)
 *          - HTTP methods (QHTTPGET, QHTTPPOST, QHTTPPUT)
 *          - Response reading (QHTTPREAD, QHTTPREADFILE)
 *          - Connection management (QHTTPSTOP)
 *
 * @note All UART access is performed through RIL_SendATCmd/RIL_SendBinaryData.
 *       This library has no direct HAL dependencies and does not conflict
 *       with other RIL modules like ril_mqtt_client.
 *
 * @dependencies ril.h, ril_file.h
 * @author Sepahtan Project Team
 * @version 1.0
 */

#ifndef RIL_HTTP_CLIENT_H
#define RIL_HTTP_CLIENT_H

#include "ril.h"
#include "ril_file.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Callback function for HTTP data chunk processing
 * @param chunk Pointer to data chunk
 * @param len Length of data chunk in bytes
 * @param ctx User-provided context pointer
 * @return true if chunk is processed successfully, false otherwise
 */
typedef bool (*RIL_HTTP_ChunkCB)(const uint8_t* chunk, uint32_t len, void* ctx);

/**
 * @brief HTTP operation error codes
 * @details These error codes correspond to Quectel modem HTTP errors
 *          as defined in the Application Note Table-2.
 */
typedef enum {
    RIL_HTTP_ERR_CHUNK_FAILED = -2, /**< Chunk processing failed */
    RIL_HTTP_ERR = -1,              /**< Generic error */
    RIL_HTTP_OK = 0,                /**< Operation successful */

    /* Quectel-specific error codes */
    RIL_HTTP_ERR_UNKNOWN = 701,       /**< Unknown error */
    RIL_HTTP_ERR_TIMEOUT = 702,       /**< Operation timeout */
    RIL_HTTP_ERR_BUSY = 703,          /**< HTTP service busy */
    RIL_HTTP_ERR_UART_BUSY = 704,     /**< UART interface busy */
    RIL_HTTP_ERR_NET_ERROR = 710,     /**< Network error */
    RIL_HTTP_ERR_URL_ERROR = 711,     /**< Invalid URL format */
    RIL_HTTP_ERR_EMPTY_URL = 712,     /**< Empty URL provided */
    RIL_HTTP_ERR_SOCKET_READ = 717,   /**< Socket read error */
    RIL_HTTP_ERR_READ_TIMEOUT = 722,  /**< Read operation timeout */
    RIL_HTTP_ERR_RESPONSE_FAIL = 723, /**< Response processing failed */
    RIL_HTTP_ERR_NOMEM = 729,         /**< Insufficient memory */
    RIL_HTTP_ERR_ARG = 730,           /**< Invalid argument */
    RIL_HTTP_ERR_SSL_FAILED = 732,    /**< SSL/TLS operation failed */
    RIL_HTTP_ERR_UNSUPPORTED = 733    /**< Operation not supported */
} RIL_HTTP_Err;

/**
 * @brief HTTP client context structure
 * @details Maintains state and configuration for a single HTTP session.
 *          One instance should be used per HTTP connection.
 */
typedef struct {
    uint8_t cid;          /**< PDP context ID (1-7) */
    uint8_t sslctx;       /**< SSL context ID (0=HTTP, 1-5=HTTPS) */
    uint32_t lastStatus;  /**< Last HTTP status code (200, 404, etc.) */
    uint32_t lastLength;  /**< Content-Length reported by module */
    RIL_HTTP_Err lastErr; /**< Last operation error code */

    /* Configuration flags */
    bool respHdr; /**< Include response headers in read */
    bool custHdr; /**< Enable custom request headers */
    bool autoRsp; /**< Enable automatic response handling */
} RIL_HTTPClient;

/* ────────────────────────────────────────────────────────────────────── */
/*                              PUBLIC API                                 */
/* ────────────────────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Initialization Functions                        */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialize HTTP client context
 * @param cli Pointer to client context structure
 * @param cid PDP context ID (1-7)
 * @param sslctx SSL context ID (0 for HTTP, 1-5 for HTTPS)
 * @details Initializes the client structure with default values and
 *          sets the specified PDP and SSL context IDs.
 */
void RIL_HTTP_Init(RIL_HTTPClient* cli, uint8_t cid, uint8_t sslctx);

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Configuration Functions                         */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Configure PDP context ID for HTTP session
 * @param cli Pointer to client context
 * @param cid PDP context ID (1-7)
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="contextid",<cid>
 */
RIL_HTTP_Err RIL_HTTP_CfgContextId(RIL_HTTPClient* cli, uint8_t cid);

/**
 * @brief Configure SSL context for HTTPS connections
 * @param cli Pointer to client context
 * @param sslctx SSL context ID (0=HTTP, 1-5=HTTPS)
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="sslctxid",<cid>,<sslctx>
 */
RIL_HTTP_Err RIL_HTTP_CfgSSL(RIL_HTTPClient* cli, uint8_t sslctx);

/**
 * @brief Enable/disable custom request headers
 * @param cli Pointer to client context
 * @param enable If true, enable sending custom request headers from MCU
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="requestheader",<0|1>
 */
RIL_HTTP_Err RIL_HTTP_CfgReqHeader(RIL_HTTPClient* cli, bool enable);

/**
 * @brief Enable/disable response header inclusion
 * @param cli Pointer to client context
 * @param enable If true, include HTTP response headers when reading
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="responseheader",<0|1>
 */
RIL_HTTP_Err RIL_HTTP_CfgRspHeader(RIL_HTTPClient* cli, bool enable);

/**
 * @brief Configure Content-Type for request body
 * @param cli Pointer to client context
 * @param mime MIME type string (e.g., "application/json", "text/plain")
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="contenttype","<mime>"
 */
RIL_HTTP_Err RIL_HTTP_CfgContentType(RIL_HTTPClient* cli, const char* mime);

/**
 * @brief Configure User-Agent header
 * @param cli Pointer to client context
 * @param ua User-Agent string
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="useragent","<string>"
 */
RIL_HTTP_Err RIL_HTTP_CfgUserAgent(RIL_HTTPClient* cli, const char* ua);

/**
 * @brief Configure HTTP Basic Authentication
 * @param cli Pointer to client context
 * @param username Username for authentication
 * @param password Password for authentication
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="auth","user:pass"
 */
RIL_HTTP_Err RIL_HTTP_CfgAuthBasic(RIL_HTTPClient* cli, const char* username, const char* password);

/**
 * @brief Enable/disable multipart form data support
 * @param cli Pointer to client context
 * @param enable If true, enable multipart/form-data for POSTFILE/PUTFILE
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="formdata",<0|1>
 */
RIL_HTTP_Err RIL_HTTP_CfgFormData(RIL_HTTPClient* cli, bool enable);

/**
 * @brief Enable/disable connection close notifications
 * @param cli Pointer to client context
 * @param enable If true, request URC notification when HTTP connection closes
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="closedind",<0|1>
 */
RIL_HTTP_Err RIL_HTTP_CfgClosedInd(RIL_HTTPClient* cli, bool enable);

/**
 * @brief Reset all HTTP configuration to defaults
 * @param cli Pointer to client context
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPCFG="del"
 */
RIL_HTTP_Err RIL_HTTP_ResetCfg(RIL_HTTPClient* cli);

/* ══════════════════════════════════════════════════════════════════════ */
/*                            URL Management                              */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Upload URL to the modem for HTTP operations
 * @param cli Pointer to client context
 * @param url Zero-terminated URL string (max 65535 bytes)
 * @param urlTimeoutSec Timeout for URL upload operation in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Performs the following sequence:
 *          1. Send AT+QHTTPURL=<len>,<timeout>
 *          2. Wait for 'CONNECT' response
 *          3. Send URL data
 *          4. Wait for final 'OK'
 */
RIL_HTTP_Err RIL_HTTP_SetURL(RIL_HTTPClient* cli, const char* url, uint16_t urlTimeoutSec);

/* ══════════════════════════════════════════════════════════════════════ */
/*                          HTTP Request Methods                          */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Perform HTTP GET request
 * @param cli Pointer to client context
 * @param rspTimeSec Timeout for response in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPGET=<timeout>
 */
RIL_HTTP_Err RIL_HTTP_Get(RIL_HTTPClient* cli, uint16_t rspTimeSec);

/**
 * @brief Perform HTTP GET request with range
 * @param cli Pointer to client context
 * @param offset Starting byte offset for range request
 * @param len Number of bytes to request
 * @param rspTimeSec Timeout for response in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPGET=<timeout>,<offset>,<len>
 */
RIL_HTTP_Err RIL_HTTP_GetRange(RIL_HTTPClient* cli, uint32_t offset, uint32_t len,
                               uint16_t rspTimeSec);

/**
 * @brief Perform HTTP POST request with data
 * @param cli Pointer to client context
 * @param body Pointer to request body data
 * @param bodyLen Length of request body in bytes
 * @param inputTimeSec Timeout for data input in seconds
 * @param rspTimeSec Timeout for response in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPPOST followed by binary data transfer
 */
RIL_HTTP_Err RIL_HTTP_Post(RIL_HTTPClient* cli, const void* body, uint32_t bodyLen,
                           uint16_t inputTimeSec, uint16_t rspTimeSec);

/**
 * @brief Perform HTTP PUT request with data
 * @param cli Pointer to client context
 * @param body Pointer to request body data
 * @param bodyLen Length of request body in bytes
 * @param inputTimeSec Timeout for data input in seconds
 * @param rspTimeSec Timeout for response in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPPUT followed by binary data transfer
 */
RIL_HTTP_Err RIL_HTTP_Put(RIL_HTTPClient* cli, const void* body, uint32_t bodyLen,
                          uint16_t inputTimeSec, uint16_t rspTimeSec);

/**
 * @brief Perform HTTP POST request with file data
 * @param cli Pointer to client context
 * @param fileName Name of file on modem storage to send
 * @param rspTimeSec Timeout for response in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPPOSTFILE
 */
RIL_HTTP_Err RIL_HTTP_PostFile(RIL_HTTPClient* cli, const char* fileName, uint16_t rspTimeSec);

/**
 * @brief Perform HTTP PUT request with file data
 * @param cli Pointer to client context
 * @param fileName Name of file on modem storage to send
 * @param rspTimeSec Timeout for response in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPPUTFILE
 */
RIL_HTTP_Err RIL_HTTP_PutFile(RIL_HTTPClient* cli, const char* fileName, uint16_t rspTimeSec);

/* ══════════════════════════════════════════════════════════════════════ */
/*                       Response Reading Functions                       */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Stream response data line-by-line
 * @param cli Pointer to client context
 * @param cb Callback function invoked for each line
 * @param ctx User context passed to callback
 * @param waitTimeSec Timeout for read operation in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Callback is invoked once for each line (up to CR/LF).
 *          Suitable for text data with regular line breaks.
 * @note For binary data without CR/LF, use other read methods.
 */
RIL_HTTP_Err RIL_HTTP_ReadLineStream(RIL_HTTPClient* cli, RIL_HTTP_ChunkCB cb, void* ctx,
                                     uint16_t waitTimeSec);

/**
 * @brief Read full response into user-provided buffer
 * @param cli Pointer to client context
 * @param buf User buffer for response data
 * @param bufLen Capacity of user buffer in bytes
 * @param actualLen Pointer to store actual bytes read (can be NULL)
 * @param waitTimeSec Timeout for read operation in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Suitable for large responses (e.g., images, videos) that fit in memory.
 */
RIL_HTTP_Err RIL_HTTP_ReadToBuf(RIL_HTTPClient* cli, uint8_t* buf, uint32_t bufLen,
                                uint32_t* actualLen, uint16_t waitTimeSec);

/**
 * @brief Save response body directly to modem file system
 * @param cli Pointer to client context
 * @param fileName Destination filename on modem storage
 * @param waitTimeSec Timeout for operation in seconds
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPREADFILE to save response directly to file
 */
RIL_HTTP_Err RIL_HTTP_ReadFile(RIL_HTTPClient* cli, const char* fileName, uint16_t waitTimeSec);

/**
 * @brief Stream previously saved file in chunks
 * @param cli Pointer to client context
 * @param fileName Name of file on modem storage to read
 * @param chunkSize Number of bytes per callback invocation
 * @param cb Callback function for data chunks
 * @param ctx User context passed to callback
 * @return RIL_HTTP_OK on success, error code on failure
 * @details Reads a file saved by RIL_HTTP_ReadFile() chunk by chunk.
 *          Uses QFREAD commands internally for efficient streaming.
 */
RIL_HTTP_Err RIL_HTTP_ReadFileStream(RIL_HTTPClient* cli, const char* fileName, uint32_t chunkSize,
                                     RIL_HTTP_ChunkCB cb, void* ctx);

/* ══════════════════════════════════════════════════════════════════════ */
/*                        Connection Management                           */
/* ══════════════════════════════════════════════════════════════════════ */

/**
 * @brief Stop/abort current HTTP operation
 * @param cli Pointer to client context
 * @return RIL_HTTP_OK on success, error code on failure
 * @note Executes AT+QHTTPSTOP to terminate current HTTP session
 */
RIL_HTTP_Err RIL_HTTP_Stop(RIL_HTTPClient* cli);

#ifdef __cplusplus
}
#endif

#endif /* RIL_HTTP_CLIENT_H */
