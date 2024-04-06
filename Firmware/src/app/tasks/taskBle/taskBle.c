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


// Prototype functions ---------------------------------------------------------
tBleStatus ble_init();
tBleStatus ble_makeDiscoverable();
tBleStatus ble_addService();
const char* ble_statusToStr(tBleStatus status);

// Global variables ------------------------------------------------------------
static TaskHandle_t _taskBleHandle = NULL;

// Implemented functions -------------------------------------------------------
void taskBleCode(__attribute__((unused)) void *parameters)
{
    _taskBleHandle = xTaskGetCurrentTaskHandle();

    tBleStatus ret = BlueNRG_Stack_Initialization(&BlueNRG_Stack_Init_params);
    boardPrintf("Ble stack init: %s\r\n", ble_statusToStr(ret));

    ret = ble_init();
    boardPrintf("Ble init: %s\r\n", ble_statusToStr(ret));

    ret = ble_addService();
    boardPrintf("Ble add service: %s\r\n", ble_statusToStr(ret));


    ret = ble_makeDiscoverable();
    boardPrintf("Ble make discoverable: %s\r\n", ble_statusToStr(ret));
    
    while (1)
    {
        BTLE_StackTick();
        if(BlueNRG_Stack_Perform_Deep_Sleep_Check() != SLEEPMODE_RUNNING)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }   
}

void BLE_IT_HANDLER()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    RAL_Isr();
    vTaskNotifyGiveFromISR( _taskBleHandle, &xHigherPriorityTaskWoken );
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

tBleStatus ble_init()
{
    tBleStatus ret;
    uint16_t service_handle;
    uint16_t dev_name_char_handle;
    uint16_t appearance_char_handle;
    uint8_t device_name[] = "BlueNRG Lock";
    uint8_t bdaddr[] =
    {
        ROM_INFO->UNIQUE_ID_1,
        ROM_INFO->UNIQUE_ID_2,
        ROM_INFO->UNIQUE_ID_3,
        ROM_INFO->UNIQUE_ID_4,
        ROM_INFO->UNIQUE_ID_5,
        ROM_INFO->UNIQUE_ID_6
    };
    
    // Configure BLE device public address
    ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bdaddr);
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
    ret = aci_gap_init(GAP_PERIPHERAL_ROLE, 0x02, sizeof(device_name)-1,
                       &service_handle, &dev_name_char_handle, &appearance_char_handle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Update device name
    ret = aci_gatt_update_char_value_ext(0, service_handle, dev_name_char_handle, 0,
                                         sizeof(device_name)-1, 0,
                                         sizeof(device_name)-1, device_name);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;



    // Update Appearance
    // - Category (bits 15 to 6) : 0x01C Access Control
    // - Sub-categor (bits 5 to 0) : 0x04 Access Lock
    // - Value : 0x0704
    uint16_t appearance_val = 0x0704;
    ret = aci_gatt_update_char_value(service_handle, appearance_char_handle, 0,
                                     sizeof(appearance_val), (uint8_t*)&appearance_val);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;





    // TODO et si il y a une coupur de courent la conection est perdue ?????????????????????????????????????????????????????????
//    ret = aci_gap_clear_security_db();
//    if (ret != BLE_STATUS_SUCCESS)
//        return ret;




    // Set the proper security I/O capability
    ret = aci_gap_set_io_capability(IO_CAP_DISPLAY_ONLY);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Set the proper security I/O authentication
    ret = aci_gap_set_authentication_requirement(BONDING,
                                                 MITM_PROTECTION_REQUIRED,
                                                 SC_IS_SUPPORTED,
                                                 KEYPRESS_IS_NOT_SUPPORTED,
                                                 7,
                                                 16,
                                                 USE_FIXED_PIN_FOR_PAIRING,
                                                 123456,
                                                 0x00);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    return BLE_STATUS_SUCCESS;
}




// #include <string.h>

uint16_t lockServHandle;
uint16_t unlockCharHandle;
//uint16_t unlockDescHandle;
uint16_t stateCharHandle;

// Define the required Services & Characteristics & Characteristics Descriptors
tBleStatus ble_addService()
{
//    Service_UUID_t service_uuid;
//    Char_UUID_t char_uuid;
#if 0
    tBleStatus ret;
    //    aci_gatt_add_service();
    //    aci_gatt_add_char();
    //    aci_gatt_add_char_desc();

    service_uuid.Service_UUID_16    = 0x1815;
    char_uuid.Char_UUID_16          = 0x2AE2;

    ret = aci_gatt_add_service(UUID_TYPE_16, &service_uuid, PRIMARY_SERVICE, 6, &lockServHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret =  aci_gatt_add_char(lockServHandle, UUID_TYPE_16, &char_uuid, 1, CHAR_PROP_WRITE, ATTR_PERMISSION_ENCRY_WRITE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                            16, 0, &unlockCharHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

#else
    tBleStatus ret;

    // UUID from https://www.famkruithof.net/uuid/uuidgen
    // 44707b20-3459-11ee-aea4-0800200c9a66
    // 44707b21-3459-11ee-aea4-0800200c9a66
    // 44707b22-3459-11ee-aea4-0800200c9a66
    Service_UUID_t servUuid     = { .Service_UUID_128 = {0x66,0x9a,0x0c,0x20,0x00,0x08,0xa4,0xae,0xee,0x11,0x59,0x34,0x20,0x7b,0x70,0x44}};
    Char_UUID_t charUuidUnlock  = { .Char_UUID_128    = {0x66,0x9a,0x0c,0x20,0x00,0x08,0xa4,0xae,0xee,0x11,0x59,0x34,0x21,0x7b,0x70,0x44}};
    Char_UUID_t charUuidState   = { .Char_UUID_128    = {0x66,0x9a,0x0c,0x20,0x00,0x08,0xa4,0xae,0xee,0x11,0x59,0x34,0x22,0x7b,0x70,0x44}};

    ret = aci_gatt_add_service(UUID_TYPE_128, &servUuid, PRIMARY_SERVICE, 12, &lockServHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret =  aci_gatt_add_char(lockServHandle, UUID_TYPE_128, &charUuidUnlock, 1,
                            CHAR_PROP_WRITE,                    ATTR_PERMISSION_ENCRY_WRITE,    GATT_NOTIFY_ATTRIBUTE_WRITE,
                            16,     0, &unlockCharHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret =  aci_gatt_add_char(lockServHandle, UUID_TYPE_128, &charUuidState, 1,
                            CHAR_PROP_READ|CHAR_PROP_NOTIFY,    ATTR_PERMISSION_NONE,           GATT_DONT_NOTIFY_EVENTS,
                            16,     0,  &stateCharHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;



//    Char_Desc_Uuid_t descUuidUnlock   = { .Char_UUID_128    = {0x66,0x9a,0x0c,0x20,0x00,0x08,0xa4,0xae,0xee,0x11,0x59,0x34,0x23,0x7b,0x70,0x44}};
//    uint8_t descUnlock[] = "This is a descriptor of Unlock";
//    ret = aci_gatt_add_char_desc(lockServHandle, unlockCharHandle, UUID_TYPE_128, &descUuidUnlock,
//                                 sizeof(descUnlock)-1, sizeof(descUnlock)-1, descUnlock,
//                                0, ATTR_ACCESS_READ_ONLY, 0, 16, 0, &unlockDescHandle);
//    if (ret != BLE_STATUS_SUCCESS)
//        return ret;


#endif
    return BLE_STATUS_SUCCESS;
}

tBleStatus ble_makeDiscoverable()
{
    tBleStatus ret;
    uint8_t local_name[] = " BlueNRG Lock";
    local_name[0] = AD_TYPE_COMPLETE_LOCAL_NAME;

    /* Disable scan response: passive scan */
    ret = hci_le_set_scan_response_data(0, NULL);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gap_set_discoverable(ADV_IND, 100, 100, RESOLVABLE_PRIVATE_ADDR,
                                   NO_WHITE_LIST_USE,
                                   sizeof(local_name)-1,
                                   local_name,
                                   0, NULL, 0, 0);
//    ret = aci_gap_set_undirected_connectable(100, 100, RESOLVABLE_PRIVATE_ADDR, NO_WHITE_LIST_USE);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    return BLE_STATUS_SUCCESS;
}

const char* ble_statusToStr(tBleStatus status)
{
    switch(status)
    {
        // Standard Error Codes
        case BLE_STATUS_SUCCESS:                        return "success ";
        case BLE_ERROR_UNKNOWN_HCI_COMMAND:             return "unknown hci command";
        case BLE_ERROR_UNKNOWN_CONNECTION_ID:           return "unknown connextion id";
        case BLE_ERROR_HARDWARE_FAILURE:                return "hardware failure";
        case BLE_ERROR_AUTHENTICATION_FAILURE:          return "authentication failure";
        case BLE_ERROR_KEY_MISSING:                     return "key missing";
        case BLE_ERROR_MEMORY_CAPACITY_EXCEEDED:        return "memory capacity exceeded";
        case BLE_ERROR_CONNECTION_TIMEOUT:              return "connection timeout";
        case BLE_ERROR_COMMAND_DISALLOWED:              return "command disallowed";
        case BLE_ERROR_UNSUPPORTED_FEATURE:             return "unsupported feature";
        case BLE_ERROR_INVALID_HCI_CMD_PARAMS:          return "invalid hci cmd params";
        case BLE_ERROR_TERMINATED_REMOTE_USER:          return "terminated remote user";
        case BLE_ERROR_TERMINATED_LOCAL_HOST:           return "terminated local_host";
        case BLE_ERROR_UNSUPP_RMT_FEATURE:              return "unsupp rmt feature";
        case BLE_ERROR_UNSPECIFIED:                     return "unspecified";
        case BLE_ERROR_PROCEDURE_TIMEOUT:               return "procedure timeout";
        case BLE_ERROR_INSTANT_PASSED:                  return "instant passed";
        case BLE_ERROR_PARAMETER_OUT_OF_RANGE:          return "parameter out of range";
        case BLE_ERROR_HOST_BUSY_PAIRING:               return "host busy pairing";
        case BLE_ERROR_CONTROLLER_BUSY:                 return "controller busy";
        case BLE_ERROR_DIRECTED_ADVERTISING_TIMEOUT:    return "directed advertising timeout";
        case BLE_ERROR_CONNECTION_END_WITH_MIC_FAILURE: return "connection end with mic failure";
        case BLE_ERROR_CONNECTION_FAILED_TO_ESTABLISH:  return "connection failed to establish";

        // Generic/System error codes
        case BLE_STATUS_UNKNOWN_CONNECTION_ID:           return "unknown connection id";
        case BLE_STATUS_FAILED:                          return "failed";
        case BLE_STATUS_INVALID_PARAMS:                  return "invalid params";
        case BLE_STATUS_BUSY:                            return "busy";
        case BLE_STATUS_PENDING:                         return "pending";
        case BLE_STATUS_NOT_ALLOWED:                     return "not allowed";
        case BLE_STATUS_ERROR:                           return "error";
        case BLE_STATUS_OUT_OF_MEMORY:                   return "out of memory";

        // L2CAP error codes
        case BLE_STATUS_INVALID_CID:                     return "invalid cid";

        // Security Manager error codes
        case BLE_STATUS_DEV_IN_BLACKLIST:                return "dev in blacklist";
        case BLE_STATUS_CSRK_NOT_FOUND:                  return "csrk not found";
        case BLE_STATUS_IRK_NOT_FOUND:                   return "irk not found";
        case BLE_STATUS_DEV_NOT_FOUND:                   return "dev not found";
        case BLE_STATUS_SEC_DB_FULL:                     return "sec db full";
        case BLE_STATUS_DEV_NOT_BONDED:                  return "dev not bonded";
        case BLE_INSUFFICIENT_ENC_KEYSIZE:               return "cient enc keysize";

        // Gatt layer Error Codes
        case BLE_STATUS_INVALID_HANDLE:                  return "invalid handle";
        case BLE_STATUS_OUT_OF_HANDLE:                   return "out of handle";
        case BLE_STATUS_INVALID_OPERATION:               return "invalid operation";
        case BLE_STATUS_CHARAC_ALREADY_EXISTS:           return "charac already exists";
        case BLE_STATUS_INSUFFICIENT_RESOURCES:          return "insufficient resources";
        case BLE_STATUS_SEC_PERMISSION_ERROR:            return "satisfy permission error";

        // GAP layer Error Codes
        case BLE_STATUS_ADDRESS_NOT_RESOLVED:            return "address not resolved";

        // Link Layer error Codes
        case BLE_STATUS_NO_VALID_SLOT:                   return "no valid slot";
        case BLE_STATUS_SCAN_WINDOW_SHORT:               return "scan window short";
        case BLE_STATUS_NEW_INTERVAL_FAILED:             return "new interval failed";
        case BLE_STATUS_INTERVAL_TOO_LARGE:              return "interval too large";
        case BLE_STATUS_LENGTH_FAILED:                   return "length failed";

        // flash error codes
        case FLASH_READ_FAILED:                          return "Flash read failed";
        case FLASH_WRITE_FAILED:                         return "Flash write failed";
        case FLASH_ERASE_FAILED:                         return "Flash erase failed";

        // Profiles Library Error Codes
        case BLE_STATUS_TIMEOUT:                         return "timeout";
        case BLE_STATUS_PROFILE_ALREADY_INITIALIZED:     return "profile already initialized";
        case BLE_STATUS_NULL_PARAM:                      return "null param";

        default: return "Unknow error";
    }
    return "";
}
