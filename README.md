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
- **Thread-Safe Operations**: Mutex-protected operations for multi-threaded environments
- **Stream Management**: Efficient UART data streaming with DMA support
- **Error Handling**: Comprehensive error reporting and recovery
- **Logging System**: Configurable debug logging with multiple levels
- **URC Support**: Non-blocking Unsolicited Result Code handling for real-time events
- **Power Management**: Automatic power cycle and restart support with callback interface

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
- CMSIS-RTOS2 (optional, for thread-safe operations when `RIL_USE_OS=1`)
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

**Note**: Polling mode is not supported as it would block the main thread. The library requires DMA or Interrupt mode for proper operation.

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
    // Parameters: UART handle, URC callback, power command callback, init result callback
    // Note: powerCommandCb and initialResultCb can be NULL if not needed
    if (RIL_initialize(&huart1, NULL, NULL, NULL) != RIL_AT_SUCCESS)
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

### HTTP POST Example

```c
#include "ril_http.h"

void http_post_example(void)
{
    RIL_HTTPClient client;
    
    // Initialize HTTP client
    RIL_HTTP_Init(&client, 1, 0);
    RIL_HTTP_CfgContextId(&client, 1);
    
    // Configure content type
    RIL_HTTP_CfgContentType(&client, RIL_HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URL_ENCODED);
    
    // Set URL
    if (RIL_HTTP_SetURL(&client, "http://httpbin.org/post", 30) == RIL_HTTP_OK)
    {
        // POST data
        const char* post_data = "field1=value1&field2=value2";
        if (RIL_HTTP_Post(&client, post_data, strlen(post_data), 10, 30) == RIL_HTTP_OK)
        {
            // Read response
            uint8_t buffer[1024];
            uint32_t actual_len;
            if (RIL_HTTP_ReadToBuf(&client, buffer, sizeof(buffer), &actual_len, 30) == RIL_HTTP_OK)
            {
                logInfo("POST Response: %.*s", actual_len, buffer);
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
RIL_ATSndError RIL_initialize(UART_HandleTypeDef* uart, 
                              RIL_URCIndicationCallback urcCb,
                              RIL_PowerCommandCallback powerCommandCb,
                              RIL_InitialResultCallback initialResultCb);
RIL_ATSndError RIL_deInitialize(void);
void RIL_ServiceRoutine(void);
bool RIL_IsModulePowered(void);
```

**Note**: `powerCommandCb` and `initialResultCb` can be `NULL` if not needed. The power command callback is used for automatic power cycle during initialization failures.

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

### Module APIs

For detailed API documentation, refer to the header files:
- **SMS**: `ril_sms.h` - Send/receive SMS messages
- **Network**: `ril_network.h` - Network registration, APN configuration, signal quality
- **HTTP**: `ril_http.h` - HTTP/HTTPS client with GET, POST, PUT, file operations
- **MQTT**: `ril_mqtt_client.h` - MQTT 3.1/3.1.1 client with SSL support
- **Socket**: `ril_socket.h` - TCP/UDP socket operations
- **SIM**: `ril_sim.h` - SIM card management
- **File**: `ril_file.h` - File system operations on modem storage

## Examples

### Complete IoT Application

```c
#include "ril.h"
#include "ril_network.h"
#include "ril_http.h"
#include "ril_mqtt_client.h"

void iot_application(void)
{
    // Initialize RIL
    RIL_initialize(&huart1, NULL, NULL, NULL);
    
    // Configure network and wait for registration
    RIL_NW_SetAPN(1, RIL_NW_ContextType_IPV4, "internet");
    RIL_NW_OpenPDPContext(1);
    // ... wait for network registration ...
    
    // Send HTTP POST request
    RIL_HTTPClient http_client;
    RIL_HTTP_Init(&http_client, 1, 0);
    RIL_HTTP_SetURL(&http_client, "https://api.thingspeak.com/update", 30);
    RIL_HTTP_Post(&http_client, "field1=25.6", 11, 10, 30);
    
    // Connect to MQTT and publish data
    RIL_MQTTClient mqtt_client;
    RIL_MQTT_InitClient(&mqtt_client, "stm32_client", NULL, NULL, 60, true);
    RIL_MQTT_Open(&mqtt_client, "broker.hivemq.com", 1883);
    RIL_MQTT_Connect(&mqtt_client);
    RIL_MQTT_Publish(&mqtt_client, 1, "sensors/data", payload, len, QOS_1, false);
}
```

## Advanced Features

### Thread Safety

When `RIL_USE_OS=1` is defined, the library provides thread-safe operations using mutex protection. Requires CMSIS-RTOS2 and RTOS kernel initialization before `RIL_initialize()`.

### Power Management

The library supports automatic power management through callbacks. Provide a `RIL_PowerCommandCallback` to enable automatic power cycle during initialization failures (up to 3 retries).

```c
void power_callback(RIL_PowerCommand cmd, uint32_t delay_ms) {
    switch(cmd) {
        case RIL_POWER_COMMAND_RESTART:
            HAL_GPIO_WritePin(MODEM_PWR_GPIO_Port, MODEM_PWR_Pin, GPIO_PIN_RESET);
            HAL_Delay(delay_ms);
            HAL_GPIO_WritePin(MODEM_PWR_GPIO_Port, MODEM_PWR_Pin, GPIO_PIN_SET);
            break;
        // ... other cases
    }
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
#define RIL_TX_STREAM_SIZE 512  // Transmit buffer size (increased from 256)
```

#### OS Support (Thread Safety)
```c
#define RIL_USE_OS 1  // Enable thread-safe operations (requires CMSIS-RTOS2)
```

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
   - If using power management, verify power command callback is properly implemented
   - Check if modem responds to AT commands using `RIL_IsModulePowered()`

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

We welcome contributions! Please fork the repository, create a feature branch, make your changes with proper documentation, test thoroughly, and submit a pull request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For technical support, create an issue on GitHub or check the troubleshooting section above.

---

**Note**: This library is designed for embedded systems and requires careful consideration of memory usage, power consumption, and real-time constraints. Always test thoroughly in your specific application environment.


