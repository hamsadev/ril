# RIL (Radio Interface Layer) Library

A comprehensive C library for interfacing with Quectel cellular modems (EC200U, EG915U, etc.) on STM32 microcontrollers. This library provides a high-level abstraction layer for AT command communication, making it easy to implement cellular connectivity features in embedded applications.

## Table of Contents

- [Features](#features)
- [Supported Modems](#supported-modems)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Configuration](#configuration)
- [Error Handling](#error-handling)
- [Contributing](#contributing)
- [License](#license)

## Features

### Core Functionality
- **AT Command Interface**: Synchronous blocking AT command execution
- **Stream Management**: Efficient UART data streaming with DMA support
- **Error Handling**: Comprehensive error reporting and recovery
- **Logging System**: Configurable debug logging with multiple levels
- **URC Support**: Non-blocking Unsolicited Result Code handling for real-time events

### Communication Modules
- **SMS**: Send/receive text messages, PDU support, concatenated SMS
- **Network**: GPRS/LTE connectivity, APN configuration, signal quality
- **HTTP/HTTPS**: Full HTTP client with SSL support, file operations
- **MQTT**: MQTT 3.1/3.1.1 client with SSL and authentication
- **Socket**: TCP/UDP socket operations for custom protocols
- **Telephony**: Voice call management (dial, answer, hangup)

### System Features
- **SIM Management**: SIM card state, IMSI, CCID retrieval
- **System Info**: IMEI, firmware version, battery status
- **File Operations**: File system access on modem storage
- **Date/Time**: NTP synchronization and time management

## Supported Modems

This library is designed for Quectel cellular modems:
- **EC200U**: 2G/3G/4G LTE Cat-1 module
- **EG915U**: 2G/3G/4G LTE Cat-1 module
- **EC200A**: 2G/3G/4G LTE Cat-1 module
- **EC600U**: 2G/3G/4G LTE Cat-1 module

*Note: While primarily tested with EC200U, the library should work with other Quectel modems that support similar AT command sets.*

## Prerequisites

### Hardware Requirements
- STM32 microcontroller (tested on STM32F407ZGT6)
- Quectel cellular modem (EC200U/EG915U)

### Software Requirements
- Any IDE and compiler (tested with MDK-ARM Keil)
- STM32 HAL Library
- Basic understanding of AT commands

### Dependencies
- STM32 HAL Library
- CMSIS (Cortex Microcontroller Software Interface Standard)
- [Param Library](https://github.com/Ali-Mirghasemi/Param) - String parameter parsing
- [Str Library](https://github.com/Ali-Mirghasemi/Str) - String manipulation functions
- [Stream Library](https://github.com/Ali-Mirghasemi/Stream) - Stream buffer implementation
- [UARTStream Library](https://github.com/Ali-Mirghasemi/UARTStream) - UART stream implementation

## Installation

### 1. Clone the Repository
```bash
git clone https://github.com/hamsadev/ril.git
cd RIL
```

### 2. Install Dependencies
Clone the required libraries:
```bash
git clone https://github.com/Ali-Mirghasemi/Param.git
git clone https://github.com/Ali-Mirghasemi/Str.git
git clone https://github.com/Ali-Mirghasemi/Stream.git
git clone https://github.com/Ali-Mirghasemi/UARTStream.git
```

### 3. Copy Source Files
Copy the following directories to your STM32 project:
```
SourceCode/
├── inc/          # RIL header files
└── src/          # RIL source files

# Also copy dependency libraries:
Param/Src/
Str/Src/
Stream/Src/
UARTStream/Src/
```

### 4. Include in Project
Add the source files to your project and include the header directory in your compiler settings.

### 5. Configure UART
Ensure your UART is properly configured for modem communication:
- Baud rate: 115200 (default) or as per modem specification
- Data bits: 8
- Stop bits: 1
- Parity: None
- Flow control: None
- **DMA or Interrupt mode must be enabled for both TX and RX**

### UART Configuration Details

The RIL library requires either DMA or Interrupt mode to be enabled for proper operation. The RIL library handles all UART communication internally through the UART handle passed to `RIL_initialize(&huart1, NULL)`.

**Note**: Polling mode is not supported as it would block the main thread.

## Quick Start

### Basic Initialization

```c
#include "ril.h"
#include "log.h"

// UART handle for modem communication
extern UART_HandleTypeDef huart1;

int main(void)
{
    // Initialize HAL
    HAL_Init();
    SystemClock_Config();
    
    // Initialize UART
    MX_USART1_UART_Init();
    
    // Initialize logging
    SerialLog_Init(&huart5);  // Use different UART for debug
    
    // Initialize RIL
    if (RIL_initialize(&huart1, NULL) != RIL_AT_SUCCESS)
    {
        logError("RIL initialization failed");
        while(1);
    }
    
    logInfo("RIL initialized successfully");
    
    // Main loop
    while (1)
    {
        // Call service routine regularly to handle URCs (non-blocking)
        RIL_ServiceRoutine();
        
        // Your application code here
        // Note: RIL_SendATCmd() calls are blocking and will wait for response
    }
}
```

### Send SMS Example

```c
#include "ril_sms.h"

void send_sms_example(void)
{
    char phone_number[] = "+1234567890";
    char message[] = "Hello from STM32!";
    
    uint32_t msg_ref;
    int32_t result = RIL_SMS_SendSMS_Text(
        phone_number, 
        strlen(phone_number),
        LIB_SMS_CHARSET_GSM7,  // GSM 7-bit encoding
        (uint8_t*)message, 
        strlen(message),
        &msg_ref
    );
    
    if (result == RIL_AT_SUCCESS)
    {
        logInfo("SMS sent successfully, ref: %lu", msg_ref);
    }
    else
    {
        logError("SMS send failed: %d", result);
    }
}
```

### HTTP GET Example

```c
#include "ril_http.h"

void http_get_example(void)
{
    RIL_HTTPClient client;
    
    // Initialize HTTP client
    RIL_HTTP_Init(&client, 1, 0);  // Context ID 1, no SSL
    
    // Configure client
    RIL_HTTP_CfgContextId(&client, 1);
    RIL_HTTP_CfgUserAgent(&client, "STM32-RIL/1.0");
    
    // Set URL
    if (RIL_HTTP_SetURL(&client, "http://httpbin.org/get", 30) == RIL_HTTP_OK)
    {
        // Perform GET request
        if (RIL_HTTP_Get(&client, 30) == RIL_HTTP_OK)
        {
            // Read response
            uint8_t buffer[1024];
            uint32_t actual_len;
            
            if (RIL_HTTP_ReadToBuf(&client, buffer, sizeof(buffer), 
                                  &actual_len, 30) == RIL_HTTP_OK)
            {
                logInfo("HTTP Response: %.*s", actual_len, buffer);
            }
        }
    }
}
```

## API Reference

### Understanding AT Commands vs URCs

The RIL library handles two types of modem communication:

#### AT Commands (Synchronous/Blocking)
- **Purpose**: Send commands to modem and wait for response
- **Behavior**: `RIL_SendATCmd()` blocks until timeout or response received
- **Use cases**: Send SMS, make HTTP requests, configure settings
- **Example**: `RIL_SMS_SendSMS_Text()` internally uses blocking AT commands

#### URCs - Unsolicited Result Codes (Asynchronous/Non-blocking)
- **Purpose**: Handle incoming events from modem (incoming SMS, network status changes, etc.)
- **Behavior**: Processed by `RIL_ServiceRoutine()` without blocking
- **Use cases**: Real-time event handling, status monitoring
- **Example**: Incoming SMS notifications, network registration status

```c
// Blocking AT command - waits for response
RIL_ATSndError result = RIL_SMS_SendSMS_Text(number, len, charset, msg, msg_len, &ref);

// Non-blocking URC processing - call regularly in main loop
RIL_ServiceRoutine();  // Handles incoming URCs without blocking
```

### Core RIL Functions

#### Initialization
```c
RIL_ATSndError RIL_initialize(UART_HandleTypeDef* uart, RIL_URCIndicationCallback urcCb);
RIL_ATSndError RIL_deInitialize(void);
void RIL_ServiceRoutine(void);
```

#### AT Command Interface
```c
// Synchronous blocking AT command execution
RIL_ATSndError RIL_SendATCmd(const char* atCmd, uint32_t atCmdLen,
                             Callback_ATResponse atRsp_callBack, void* userData,
                             uint32_t timeOut);

// Binary data transmission (also blocking)
RIL_ATSndError RIL_SendBinaryData(const uint8_t* data, uint32_t dataLen,
                                  Callback_ATResponse atRsp_callBack, void* userData,
                                  uint32_t timeOut);

// Non-blocking URC handling (must be called regularly)
void RIL_ServiceRoutine(void);
```

### SMS Module (ril_sms.h)

#### Send SMS
```c
int32_t RIL_SMS_SendSMS_Text(char* pNumber, uint8_t uNumberLen, 
                             LIB_SMS_CharSetEnum eCharset,
                             uint8_t* pMsg, uint32_t uMsgLen, uint32_t* pMsgRef);
int32_t RIL_SMS_SendSMS_PDU(char* pPDUStr, uint32_t uPDUStrLen, uint32_t* pMsgRef);
```

#### Read SMS
```c
int32_t RIL_SMS_ReadSMS_Text(uint32_t uIndex, LIB_SMS_CharSetEnum eCharset,
                             ST_RIL_SMS_TextInfo* pTextInfo);
int32_t RIL_SMS_ReadSMS_PDU(uint32_t uIndex, ST_RIL_SMS_PDUInfo* pPDUInfo);
```

### Network Module (ril_network.h)

#### Network Status
```c
RIL_ATSndError RIL_NW_GetGSMState(RIL_NW_State* stat);
RIL_ATSndError RIL_NW_GetGPRSState(RIL_NW_State* stat);
RIL_ATSndError RIL_NW_GetSignalQuality(uint8_t* rssi, uint8_t* ber);
```

#### APN Configuration
```c
RIL_ATSndError RIL_NW_SetAPN(uint8_t pdp_id, RIL_NW_ContextType contextType, char* apn);
RIL_ATSndError RIL_NW_OpenPDPContext(uint8_t pdp_id);
RIL_ATSndError RIL_NW_ClosePDPContext(uint8_t pdp_id);
```

### HTTP Module (ril_http.h)

#### Client Management
```c
void RIL_HTTP_Init(RIL_HTTPClient* cli, uint8_t cid, uint8_t sslctx);
RIL_HTTP_Err RIL_HTTP_CfgContextId(RIL_HTTPClient* cli, uint8_t cid);
RIL_HTTP_Err RIL_HTTP_CfgSSL(RIL_HTTPClient* cli, uint8_t sslctx);
```

#### HTTP Operations
```c
RIL_HTTP_Err RIL_HTTP_SetURL(RIL_HTTPClient* cli, const char* url, uint16_t urlTimeoutSec);
RIL_HTTP_Err RIL_HTTP_Get(RIL_HTTPClient* cli, uint16_t rspTimeSec);
RIL_HTTP_Err RIL_HTTP_Post(RIL_HTTPClient* cli, const void* body, uint32_t bodyLen,
                           uint16_t inputTimeSec, uint16_t rspTimeSec);
```

### MQTT Module (ril_mqtt_client.h)

#### Client Configuration
```c
RIL_MQTT_Err RIL_MQTT_InitClient(RIL_MQTTClient* client, const char* clientId,
                                 const char* username, const char* password,
                                 uint16_t keepAlive, bool cleanSession);
RIL_MQTT_Err RIL_MQTT_ConfigSSL(RIL_MQTTClient* client, MQTT_SSLMode sslMode, uint8_t sslCtxId);
```

#### MQTT Operations
```c
RIL_MQTT_Err RIL_MQTT_Open(RIL_MQTTClient* client, const char* host, uint16_t port);
RIL_MQTT_Err RIL_MQTT_Connect(RIL_MQTTClient* client);
RIL_MQTT_Err RIL_MQTT_Publish(RIL_MQTTClient* client, uint16_t msgId, const char* topic,
                              const uint8_t* payload, uint32_t length, Enum_Qos qos, bool retain);
```

## Examples

### Complete IoT Application

```c
#include "ril.h"
#include "ril_network.h"
#include "ril_http.h"
#include "ril_mqtt_client.h"

void iot_application(void)
{
    // 1. Initialize RIL
    RIL_initialize(&huart1, NULL);
    
    // 2. Check SIM status
    Enum_SIMState sim_state;
    RIL_SIM_GetSimState(&sim_state);
    if (sim_state != SIM_STAT_READY) {
        logError("SIM not ready");
        return;
    }
    
    // 3. Configure network
    RIL_NW_SetAPN(1, RIL_NW_ContextType_IPV4, "internet");
    RIL_NW_OpenPDPContext(1);
    
    // 4. Wait for network registration
    RIL_NW_State gsm_state, gprs_state;
    do {
        RIL_NW_GetGSMState(&gsm_state);
        RIL_NW_GetGPRSState(&gprs_state);
        HAL_Delay(1000);
    } while (gsm_state != RIL_NW_State_Registered || 
             gprs_state != RIL_NW_State_Registered);
    
    // 5. Send HTTP request
    RIL_HTTPClient http_client;
    RIL_HTTP_Init(&http_client, 1, 0);
    RIL_HTTP_SetURL(&http_client, "https://api.thingspeak.com/update", 30);
    RIL_HTTP_Post(&http_client, "field1=25.6", 11, 10, 30);
    
    // 6. Connect to MQTT broker
    RIL_MQTTClient mqtt_client;
    RIL_MQTT_InitClient(&mqtt_client, "stm32_client", NULL, NULL, 60, true);
    RIL_MQTT_Open(&mqtt_client, "broker.hivemq.com", 1883);
    RIL_MQTT_Connect(&mqtt_client);
    
    // 7. Publish sensor data
    char payload[64];
    sprintf(payload, "{\"temperature\":%.1f,\"humidity\":%.1f}", 25.6, 60.2);
    RIL_MQTT_Publish(&mqtt_client, 1, "sensors/data", (uint8_t*)payload, 
                     strlen(payload), QOS_1, false);
}
```

## Configuration

### Compile-time Configuration

#### Enable/Disable Logging
```c
#define RIL_LOG_ENABLE 1  // Enable debug logging
```

#### Buffer Sizes
```c
#define RIL_RX_STREAM_SIZE 512  // Receive buffer size
#define RIL_TX_STREAM_SIZE 256  // Transmit buffer size
```

### Runtime Configuration

#### Modem Settings
- Power management: Configure according to your power requirements
- Network selection: Automatic or manual operator selection
- APN settings: Configure based on your carrier

## Error Handling

### Error Codes

The library uses consistent error codes across all modules:

```c
typedef enum {
    RIL_AT_SUCCESS = 0,        // Operation successful
    RIL_AT_FAILED = -1,        // Operation failed
    RIL_AT_TIMEOUT = -2,       // Operation timeout
    RIL_AT_BUSY = -3,          // System busy
    RIL_AT_INVALID_PARAM = -4, // Invalid parameters
    RIL_AT_UNINITIALIZED = -5  // RIL not initialized
} RIL_ATSndError;
```

### Error Handling Best Practices

```c
RIL_ATSndError result = RIL_SMS_SendSMS_Text(number, len, charset, msg, msg_len, &ref);

switch (result) {
    case RIL_AT_SUCCESS:
        logInfo("SMS sent successfully");
        break;
    case RIL_AT_TIMEOUT:
        logWarn("SMS send timeout, retrying...");
        // Implement retry logic
        break;
    case RIL_AT_FAILED:
        logError("SMS send failed, error code: %d", RIL_AT_GetErrCode());
        break;
    default:
        logError("Unexpected error: %d", result);
        break;
}
```

## Troubleshooting

### Common Issues

1. **RIL Initialization Fails**
   - Check UART configuration
   - Verify modem power supply
   - Ensure proper baud rate

2. **Network Registration Issues**
   - Check SIM card insertion
   - Verify APN settings
   - Check signal strength

3. **HTTP/HTTPS Failures**
   - Ensure PDP context is active
   - Check SSL configuration
   - Verify URL format

4. **MQTT Connection Issues**
   - Check broker address and port
   - Verify client credentials
   - Check network connectivity

### Debug Tips

1. Enable logging to see detailed operation information
2. Use AT commands directly to test modem functionality
3. Check signal quality before attempting data operations
4. Monitor power supply stability

## Contributing

We welcome contributions to improve the RIL library. Please follow these guidelines:

1. Fork the repository
2. Create a feature branch
3. Make your changes with proper documentation
4. Test thoroughly on target hardware
5. Submit a pull request with detailed description

### Development Setup

1. Clone the repository
2. Open the example project in STM32CubeIDE
3. Configure your hardware settings
4. Build and test your changes

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For technical support and questions:
- Create an issue on GitHub
- Check the documentation and examples
- Review the troubleshooting section

---

**Note**: This library is designed for embedded systems and requires careful consideration of memory usage, power consumption, and real-time constraints. Always test thoroughly in your specific application environment.


