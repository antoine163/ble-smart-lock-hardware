/***
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 antoine163
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file taskBle.c
 * @author antoine163
 * @date 02-04-2024
 * @brief Task of BLE radio
 */

// Include ---------------------------------------------------------------------
#include "taskBle.h"
#include "board.h"
#include "itConfig.h"
#include "bleConfig.h"

#include "bluenrg1_stack.h"
#include "bluenrg1_hal.h"
#include "bluenrg1_gap.h"
#include "bluenrg1_gatt_server.h"
#include "sleep.h"
#include "sm.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Define ----------------------------------------------------------------------
#define _TASK_BLE_DEFAULT_NAME "Ble Smart Lock"
#define _TASK_BLE_NAME_MAX_SIZE 16
#define _TASK_BLE_ADV_INTERVAL_MIN_MS 1000
#define _TASK_BLE_ADV_INTERVAL_MAX_MS 1500

#define _TASK_BLE_EVENT_QUEUE_LENGTH 8

// Enum ------------------------------------------------------------------------
typedef enum
{
    BLE_EVENT_RADIO_IRQ
} taskBleEventType_t;

// Struct ----------------------------------------------------------------------
typedef struct
{
    taskBleEventType_t type;
} taskBleEvent_t;

typedef struct
{
    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventStaticQueue;
    uint8_t eventQueueStorageArea[sizeof(taskBleEvent_t) * _TASK_BLE_EVENT_QUEUE_LENGTH];

    // GAP handle
    uint16_t serviceGapHandle;
    uint16_t devNameCharGapHandle;
    uint16_t appearanceCharGapHandle;

    // APP handle
    /**
     * @brief Service handle of application.
     */
    uint16_t serviceAppHandle;

    /**
     * @brief Characteristic, state of the lock.
     *
     * This characteristic is write only, its type is uint8_t and can take the
     * following values:
     * - 0x00: Locked
     * - 0x01: Unlocked
     */
    uint16_t lockStateCharAppHandle;

    /**
     * @brief Characteristic, state of the door.
     *
     * This characteristic is read only, its type is uint8_t and can take the
     * following values:
     * - 0x00: Close
     * - 0x01: Open
     */
    uint16_t doorStateCharAppHandle;

    /**
     * @brief Characteristic, open the door electronically.
     *
     * This characteristic is write only, its type is uint8_t and can take the
     * following values:
     * - 0x01: Open the door
     */
    uint16_t openDoorCharAppHandle;

    /**
     * @brief Characteristic, ambient brightness.
     *
     * This characteristic is read only, its type is float and can take the
     * following values:
     * - 0.0% to 100.0%
     */
    uint16_t brightnessCharAppHandle;

    /**
     * @brief Characteristic, brightness threshold.
     *
     * Adjusting the brightness threshold (between day and night).
     *
     * This characteristic is read/write, its type is float and can take the
     * following values:
     * - 0.0% to 100.0%
     */
    uint16_t brightnessThCharAppHandle;
} taskBle_t;

// Prototype functions ---------------------------------------------------------
tBleStatus _taskBleInitDevice();
tBleStatus _taskBleMakeDiscoverable();
tBleStatus _taskBleAddServices();
const char *_taskBleStatusToStr(tBleStatus status);

// Global variables ------------------------------------------------------------
static taskBle_t _taskBle = {0};

// Implemented functions -------------------------------------------------------
void taskBleCodeInit()
{
    // Init event queue
    _taskBle.eventQueue = xQueueCreateStatic(_TASK_BLE_EVENT_QUEUE_LENGTH,
                                           sizeof(taskBleEvent_t),
                                           _taskBle.eventQueueStorageArea,
                                           &_taskBle.eventStaticQueue);

    // Init Ble
    tBleStatus ret = BlueNRG_Stack_Initialization(&BlueNRG_Stack_Init_params);
    if (ret != BLE_STATUS_SUCCESS)
        boardPrintf("Ble stack init: %s\r\n", _taskBleStatusToStr(ret));

    if (ret == BLE_STATUS_SUCCESS)
    {
        ret = _taskBleInitDevice();
        if (ret != BLE_STATUS_SUCCESS)
            boardPrintf("Ble init device error: %s\r\n", _taskBleStatusToStr(ret));
    }

    if (ret == BLE_STATUS_SUCCESS)
    {
        ret = _taskBleAddServices();
        if (ret != BLE_STATUS_SUCCESS)
            boardPrintf("Ble add service error: %s\r\n", _taskBleStatusToStr(ret));
    }

    if (ret == BLE_STATUS_SUCCESS)
    {
        ret = _taskBleMakeDiscoverable();
        if (ret != BLE_STATUS_SUCCESS)
            boardPrintf("Ble make discoverable error: %s\r\n", _taskBleStatusToStr(ret));
    }

    if (ret == BLE_STATUS_SUCCESS)
        boardPrintf("Ble initialised with success\r\n");
}

// Implemented functions -------------------------------------------------------
void taskBleCode(__attribute__((unused)) void *parameters)
{
    while (1)
    {
        BTLE_StackTick();
        if (BlueNRG_Stack_Perform_Deep_Sleep_Check() != SLEEPMODE_RUNNING)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }
}

void BLE_IT_HANDLER()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    RAL_Isr();

    taskBleEvent_t event = {
        .type = BLE_EVENT_RADIO_IRQ};
    xQueueSendFromISR(_taskBle.eventQueue, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

tBleStatus _taskBleInitDevice()
{
    tBleStatus ret;
    uint8_t deviceName[] = _TASK_BLE_DEFAULT_NAME;
    uint8_t bdaddr[] = {
        ROM_INFO->UNIQUE_ID_1,
        ROM_INFO->UNIQUE_ID_2,
        ROM_INFO->UNIQUE_ID_3,
        ROM_INFO->UNIQUE_ID_4,
        ROM_INFO->UNIQUE_ID_5,
        ROM_INFO->UNIQUE_ID_6};

    // Configure BLE device public address
    ret = aci_hal_write_config_data(
        CONFIG_DATA_PUBADDR_OFFSET,
        CONFIG_DATA_PUBADDR_LEN,
        bdaddr);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    /* Set the TX power -14 dBm */
    ret = aci_hal_set_tx_power_level(1, 0);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Init BLE GATT layer
    ret = aci_gatt_init();
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Init BLE GAP layer
    ret = aci_gap_init(
        GAP_PERIPHERAL_ROLE,
        PRIVACY_ENABLED,
        _TASK_BLE_NAME_MAX_SIZE,
        &_taskBle.serviceGapHandle,
        &_taskBle.devNameCharGapHandle,
        &_taskBle.appearanceCharGapHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Update device name
    ret = aci_gatt_update_char_value_ext(
        0, _taskBle.serviceGapHandle, _taskBle.devNameCharGapHandle,
        0x00, // GATT_LOCAL_UPDATE
        sizeof(deviceName) - 1,
        0, sizeof(deviceName) - 1, deviceName);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Update Appearance (from Bluetooth SIG)
    // - Category (bits 15 to 6) : 0x01C Access Control
    // - Sub-categor (bits 5 to 0) : 0x08 Access Lock
    // - Value : 0x0708 // Door Lock
    uint16_t appearanceVal = 0x0708;
    ret = aci_gatt_update_char_value(
        _taskBle.serviceGapHandle, _taskBle.appearanceCharGapHandle,
        0x00, sizeof(appearanceVal), (uint8_t *)&appearanceVal);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // TODO et si il y a une coupur de courent la conection est perdue ? Non a parament
    // TODO a appler si on veur tou re inisilsier la config par defeau ...
    //    ret = aci_gap_clear_security_db();
    //    if (ret != BLE_STATUS_SUCCESS)
    //        return ret;

    // Set the proper security I/O capability
    ret = aci_gap_set_io_capability(IO_CAP_DISPLAY_ONLY);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Set the proper security I/O authentication
    ret = aci_gap_set_authentication_requirement(
        BONDING,
        MITM_PROTECTION_REQUIRED,
        SC_IS_MANDATORY,
        KEYPRESS_IS_NOT_SUPPORTED,
        7,  // Minimum encryption key size
        16, // Maximum encryption key size
        USE_FIXED_PIN_FOR_PAIRING,
        123456, // Fixed Pin
        STATIC_RANDOM_ADDR);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    return BLE_STATUS_SUCCESS;
}

// Define the required Services & Characteristics & Characteristics Descriptors
tBleStatus _taskBleAddServices()
{
    tBleStatus ret;

    // UUID from https://www.famkruithof.net/uuid/uuidgen
    // 44707b20-3459-11ee-aea4-0800200c9a66
    // 44707b21-3459-11ee-aea4-0800200c9a66
    // 44707b22-3459-11ee-aea4-0800200c9a66
    // 44707b23-3459-11ee-aea4-0800200c9a66
    // 44707b24-3459-11ee-aea4-0800200c9a66
    // 44707b25-3459-11ee-aea4-0800200c9a66
    Service_UUID_t serviceAppUuid = {.Service_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x20, 0x7b, 0x70, 0x44}};
    Char_UUID_t lockStateCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x21, 0x7b, 0x70, 0x44}};
    Char_UUID_t doorStateCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x22, 0x7b, 0x70, 0x44}};
    Char_UUID_t openDoorCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x23, 0x7b, 0x70, 0x44}};
    Char_UUID_t brightnessCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x24, 0x7b, 0x70, 0x44}};
    Char_UUID_t brightnessThCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x25, 0x7b, 0x70, 0x44}};

    ret = aci_gatt_add_service(
        UUID_TYPE_128, &serviceAppUuid,
        PRIMARY_SERVICE,
        1 + NUM_APP_GATT_ATTRIBUTES, // 1 (service) + N characteristic
        &_taskBle.serviceAppHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &lockStateCharAppUuid,
        sizeof(uint8_t), // Maximum length of the characteristic value
        CHAR_PROP_WRITE,
        ATTR_PERMISSION_ENCRY_WRITE,
        GATT_NOTIFY_ATTRIBUTE_WRITE,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.lockStateCharAppHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &doorStateCharAppUuid,
        sizeof(uint8_t), // Maximum length of the characteristic value
        CHAR_PROP_READ | CHAR_PROP_NOTIFY,
        ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.doorStateCharAppHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &openDoorCharAppUuid,
        sizeof(uint8_t), // Maximum length of the characteristic value
        CHAR_PROP_WRITE,
        ATTR_PERMISSION_ENCRY_WRITE,
        GATT_NOTIFY_ATTRIBUTE_WRITE,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.openDoorCharAppHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &brightnessCharAppUuid,
        sizeof(float), // Maximum length of the characteristic value
        CHAR_PROP_READ,
        ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.brightnessCharAppHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &brightnessThCharAppUuid,
        sizeof(float), // Maximum length of the characteristic value
        CHAR_PROP_WRITE,
        ATTR_PERMISSION_ENCRY_WRITE,
        GATT_NOTIFY_ATTRIBUTE_WRITE,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.brightnessThCharAppHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    return BLE_STATUS_SUCCESS;
}

tBleStatus _taskBleMakeDiscoverable()
{
    tBleStatus ret;
    uint8_t localName[] = " "_TASK_BLE_DEFAULT_NAME;
    localName[0] = AD_TYPE_COMPLETE_LOCAL_NAME;

    /* Disable scan response: passive scan */
    ret = hci_le_set_scan_response_data(0, NULL);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gap_set_discoverable(
        ADV_IND,
        (_TASK_BLE_ADV_INTERVAL_MIN_MS * 1000) / 625,
        (_TASK_BLE_ADV_INTERVAL_MAX_MS * 1000) / 625,
        RESOLVABLE_PRIVATE_ADDR,
        NO_WHITE_LIST_USE,
        sizeof(localName) - 1,
        localName,
        0, NULL, 0, 0);
    //    ret = aci_gap_set_undirected_connectable(100, 100, RESOLVABLE_PRIVATE_ADDR, NO_WHITE_LIST_USE);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    return BLE_STATUS_SUCCESS;
}

const char *_taskBleStatusToStr(tBleStatus status)
{
    switch (status)
    {
    // Standard Error Codes
    case BLE_STATUS_SUCCESS:
        return "success";
    case BLE_ERROR_UNKNOWN_HCI_COMMAND:
        return "unknown hci command";
    case BLE_ERROR_UNKNOWN_CONNECTION_ID:
        return "unknown connextion id";
    case BLE_ERROR_HARDWARE_FAILURE:
        return "hardware failure";
    case BLE_ERROR_AUTHENTICATION_FAILURE:
        return "authentication failure";
    case BLE_ERROR_KEY_MISSING:
        return "key missing";
    case BLE_ERROR_MEMORY_CAPACITY_EXCEEDED:
        return "memory capacity exceeded";
    case BLE_ERROR_CONNECTION_TIMEOUT:
        return "connection timeout";
    case BLE_ERROR_COMMAND_DISALLOWED:
        return "command disallowed";
    case BLE_ERROR_UNSUPPORTED_FEATURE:
        return "unsupported feature";
    case BLE_ERROR_INVALID_HCI_CMD_PARAMS:
        return "invalid hci cmd params";
    case BLE_ERROR_TERMINATED_REMOTE_USER:
        return "terminated remote user";
    case BLE_ERROR_TERMINATED_LOCAL_HOST:
        return "terminated local_host";
    case BLE_ERROR_UNSUPP_RMT_FEATURE:
        return "unsupp rmt feature";
    case BLE_ERROR_UNSPECIFIED:
        return "unspecified";
    case BLE_ERROR_PROCEDURE_TIMEOUT:
        return "procedure timeout";
    case BLE_ERROR_INSTANT_PASSED:
        return "instant passed";
    case BLE_ERROR_PARAMETER_OUT_OF_RANGE:
        return "parameter out of range";
    case BLE_ERROR_HOST_BUSY_PAIRING:
        return "host busy pairing";
    case BLE_ERROR_CONTROLLER_BUSY:
        return "controller busy";
    case BLE_ERROR_DIRECTED_ADVERTISING_TIMEOUT:
        return "directed advertising timeout";
    case BLE_ERROR_CONNECTION_END_WITH_MIC_FAILURE:
        return "connection end with mic failure";
    case BLE_ERROR_CONNECTION_FAILED_TO_ESTABLISH:
        return "connection failed to establish";

    // Generic/System error codes
    case BLE_STATUS_UNKNOWN_CONNECTION_ID:
        return "unknown connection id";
    case BLE_STATUS_FAILED:
        return "failed";
    case BLE_STATUS_INVALID_PARAMS:
        return "invalid params";
    case BLE_STATUS_BUSY:
        return "busy";
    case BLE_STATUS_PENDING:
        return "pending";
    case BLE_STATUS_NOT_ALLOWED:
        return "not allowed";
    case BLE_STATUS_ERROR:
        return "error";
    case BLE_STATUS_OUT_OF_MEMORY:
        return "out of memory";

    // L2CAP error codes
    case BLE_STATUS_INVALID_CID:
        return "invalid cid";

    // Security Manager error codes
    case BLE_STATUS_DEV_IN_BLACKLIST:
        return "dev in blacklist";
    case BLE_STATUS_CSRK_NOT_FOUND:
        return "csrk not found";
    case BLE_STATUS_IRK_NOT_FOUND:
        return "irk not found";
    case BLE_STATUS_DEV_NOT_FOUND:
        return "dev not found";
    case BLE_STATUS_SEC_DB_FULL:
        return "sec db full";
    case BLE_STATUS_DEV_NOT_BONDED:
        return "dev not bonded";
    case BLE_INSUFFICIENT_ENC_KEYSIZE:
        return "cient enc keysize";

    // Gatt layer Error Codes
    case BLE_STATUS_INVALID_HANDLE:
        return "invalid handle";
    case BLE_STATUS_OUT_OF_HANDLE:
        return "out of handle";
    case BLE_STATUS_INVALID_OPERATION:
        return "invalid operation";
    case BLE_STATUS_CHARAC_ALREADY_EXISTS:
        return "charac already exists";
    case BLE_STATUS_INSUFFICIENT_RESOURCES:
        return "insufficient resources";
    case BLE_STATUS_SEC_PERMISSION_ERROR:
        return "satisfy permission error";

    // GAP layer Error Codes
    case BLE_STATUS_ADDRESS_NOT_RESOLVED:
        return "address not resolved";

    // Link Layer error Codes
    case BLE_STATUS_NO_VALID_SLOT:
        return "no valid slot";
    case BLE_STATUS_SCAN_WINDOW_SHORT:
        return "scan window short";
    case BLE_STATUS_NEW_INTERVAL_FAILED:
        return "new interval failed";
    case BLE_STATUS_INTERVAL_TOO_LARGE:
        return "interval too large";
    case BLE_STATUS_LENGTH_FAILED:
        return "length failed";

    // flash error codes
    case FLASH_READ_FAILED:
        return "Flash read failed";
    case FLASH_WRITE_FAILED:
        return "Flash write failed";
    case FLASH_ERASE_FAILED:
        return "Flash erase failed";

    // Profiles Library Error Codes
    case BLE_STATUS_TIMEOUT:
        return "timeout";
    case BLE_STATUS_PROFILE_ALREADY_INITIALIZED:
        return "profile already initialized";
    case BLE_STATUS_NULL_PARAM:
        return "null param";

    default:
        return "Unknow error";
    }
    return "";
}
