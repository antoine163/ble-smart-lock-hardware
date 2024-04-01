/***
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 antoine163
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

// Includes --------------------------------------------------------------------
#include "board.h"
#include "uart.h"
#include "sleep.h"
#include "bluenrg1_stack.h"
#include "ble_const.h"
#include "ble_config.h"

#include "pwm.h"
#include "adc.h"
#include "BlueNRG1_adc.h"

#include <stdio.h>

// Prototype functions ---------------------------------------------------------
tBleStatus ble_init();
tBleStatus ble_makeDiscoverable();
tBleStatus ble_addService();
const char *ble_statusToStr(tBleStatus status);
void APP_Tick();

volatile uint16_t connection_handle = 0;
uint8_t connectedPeerAddress[6];

typedef enum app_state_t
{
    APP_STATS_DISCOVERABLE,
    APP_STATS_CONNECTED,
    APP_STATS_DISCONNECTED
} app_state_t;

volatile app_state_t app_state = APP_STATS_DISCONNECTED;

// Implemented functions -------------------------------------------------------
int main()
{
    SystemInit();
    boardInit();

    uart_init();
    uart_config(UART_BAUDRATE_115200, UART_DATA_8BITS, UART_PARITY_NO, UART_STOPBIT_1);

    uart_printf(" -- BL unlocker " SW_VERSION_STR " --\r\n");

    if (ADC_SwCalibration())
        printf("No calibration points found. SW compensation cannot be done.\r\n");

    // pwm_init();
    adc_init();

    boardLedOn();

#if 0

    // Initialize the BLE stack
    tBleStatus ret = BlueNRG_Stack_Initialization(&BlueNRG_Stack_Init_params);
    uart_printf("Ble stack init: %s\r\n", ble_statusToStr(ret));

    ret = ble_init();
    uart_printf("Ble init: %s\r\n", ble_statusToStr(ret));
 
    ret = ble_addService();
    uart_printf("Ble add service: %s\r\n", ble_statusToStr(ret));


    while(1)
    {
        BTLE_StackTick();
        APP_Tick();
        //BlueNRG_Sleep(SLEEPMODE_CPU_HALT, 0, 0);
    }
#else
#define PRINT_INT(x) ((int)(x))
#define PRINT_FLOAT(x) (x > 0) ? ((int)(((x)-PRINT_INT(x)) * 1000)) : (-1 * (((int)(((x)-PRINT_INT(x)) * 1000))))

#define DELAY 20
    int cpt = 0;

    GPIO_SetBits(EN_IO_PIN);

    while (1)
    {
        float val = adcGet();

        float dc = val * 100 / 3300;

        uart_printf("ADC value: %d.%03d mV | dc: %d.%03d%% | %s\r\n",
                    PRINT_INT(val), PRINT_FLOAT(val),
                    PRINT_INT(dc), PRINT_FLOAT(dc),
                    (GPIO_ReadBit(OPENED_PIN)==Bit_SET)? "open":"close");

        if (cpt == 1000 / DELAY)
        {
            boardLight(LIGHT_RED, dc);
        }
        else if (cpt == 2000 / DELAY)
        {
            boardLight(LIGHT_GREEN, dc);
        }
        else if (cpt == 3000 / DELAY)
        {
            boardLight(LIGHT_BLUE, dc);
        }
        else if (cpt == 4000 / DELAY)
        {
            boardLight(LIGHT_WHITE, dc);
            cpt = 0;
        }

        if (cpt < 1000 / DELAY)
            pwm_setDutyCycle(dc);
        else
            pwm_setDutyCycle(100 - dc);

        tick_delay(DELAY);

        cpt++;
    }

#endif

    return 0;
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
            ROM_INFO->UNIQUE_ID_6};

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
    ret = aci_gap_init(GAP_PERIPHERAL_ROLE, 0x02, sizeof(device_name) - 1,
                       &service_handle, &dev_name_char_handle, &appearance_char_handle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Update device name
    ret = aci_gatt_update_char_value_ext(0, service_handle, dev_name_char_handle, 0,
                                         sizeof(device_name) - 1, 0,
                                         sizeof(device_name) - 1, device_name);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    // Update Appearance
    // - Category (bits 15 to 6) : 0x01C Access Control
    // - Sub-categor (bits 5 to 0) : 0x04 Access Lock
    // - Value : 0x0704
    uint16_t appearance_val = 0x0704;
    ret = aci_gatt_update_char_value(service_handle, appearance_char_handle, 0,
                                     sizeof(appearance_val), (uint8_t *)&appearance_val);
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

#include <string.h>

uint16_t lockServHandle;
uint16_t unlockCharHandle;
// uint16_t unlockDescHandle;
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
    Service_UUID_t servUuid = {.Service_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x20, 0x7b, 0x70, 0x44}};
    Char_UUID_t charUuidUnlock = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x21, 0x7b, 0x70, 0x44}};
    Char_UUID_t charUuidState = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x22, 0x7b, 0x70, 0x44}};

    ret = aci_gatt_add_service(UUID_TYPE_128, &servUuid, PRIMARY_SERVICE, 12, &lockServHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gatt_add_char(lockServHandle, UUID_TYPE_128, &charUuidUnlock, 1,
                            CHAR_PROP_WRITE, ATTR_PERMISSION_ENCRY_WRITE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                            16, 0, &unlockCharHandle);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    ret = aci_gatt_add_char(lockServHandle, UUID_TYPE_128, &charUuidState, 1,
                            CHAR_PROP_READ | CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE, GATT_DONT_NOTIFY_EVENTS,
                            16, 0, &stateCharHandle);
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
                                   sizeof(local_name) - 1,
                                   local_name,
                                   0, NULL, 0, 0);
    //    ret = aci_gap_set_undirected_connectable(100, 100, RESOLVABLE_PRIVATE_ADDR, NO_WHITE_LIST_USE);
    if (ret != BLE_STATUS_SUCCESS)
        return ret;

    return BLE_STATUS_SUCCESS;
}

void APP_Tick()
{
    static bool lastBpStatus = false;
    bool bpStatus = false; // boardReadButton(BUTTON1);
    static app_state_t lastAppState = APP_STATS_DISCONNECTED;

    if (lastAppState != app_state)
    {
        switch (app_state)
        {
        case APP_STATS_DISCOVERABLE:
        {
            break;
        }
        case APP_STATS_CONNECTED:
        {
            uart_printf("Device 0x%x%x%x%x%x%x connected\r\n",
                        connectedPeerAddress[0],
                        connectedPeerAddress[1],
                        connectedPeerAddress[2],
                        connectedPeerAddress[3],
                        connectedPeerAddress[4],
                        connectedPeerAddress[5]);
            break;
        }

        case APP_STATS_DISCONNECTED:
        {
            uart_printf("Device 0x%x%x%x%x%x%x disconnected\r\n",
                        connectedPeerAddress[0],
                        connectedPeerAddress[1],
                        connectedPeerAddress[2],
                        connectedPeerAddress[3],
                        connectedPeerAddress[4],
                        connectedPeerAddress[5]);

            // lock
            // Note: pas besoine d'appeler aci_gatt_update_char_value_ext()
            // car aci_gatt_attribute_modified_event() est appler dans tous
            // cas même si la vleur et re ecrite la même que précédament.
            // boardLedOff(LED2);

            break;
        }
        }
        lastAppState = app_state;
    }

    switch (app_state)
    {
    case APP_STATS_DISCOVERABLE:
    {
        break;
    }
    case APP_STATS_CONNECTED:
    {
        if ((bpStatus != lastBpStatus))
        {
            lastBpStatus = bpStatus;

            tBleStatus ret = aci_gatt_update_char_value_ext(connection_handle, lockServHandle, stateCharHandle, 1, 1, 0, 1, (uint8_t *)&bpStatus);
            if (ret != BLE_STATUS_SUCCESS)
                uart_printf("Ble write config data: %s\r\n", ble_statusToStr(ret));
        }

        break;
    }

    case APP_STATS_DISCONNECTED:
    {
        tBleStatus ret = ble_makeDiscoverable();
        uart_printf("Ble make discoverable: %s\r\n", ble_statusToStr(ret));

        if (ret == BLE_STATUS_SUCCESS)
            app_state = APP_STATS_DISCOVERABLE;

        break;
    }
    }
}

void aci_gatt_attribute_modified_event(uint16_t Connection_Handle,
                                       uint16_t Attr_Handle,
                                       uint16_t Offset,
                                       uint16_t Attr_Data_Length,
                                       uint8_t Attr_Data[])
{
    (void)Connection_Handle;
    (void)Offset;
    (void)Attr_Data_Length;

    if (Attr_Handle == unlockCharHandle + 1)
    {
        // if (Attr_Data[0] != 0)
        //     boardLedOn(LED2);
        // else
        //     boardLedOff(LED2);
    }
}

void hci_le_connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Role,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Conn_Interval,
                                      uint16_t Conn_Latency,
                                      uint16_t Supervision_Timeout,
                                      uint8_t Master_Clock_Accuracy)
{
    (void)Status;
    (void)Role;
    (void)Peer_Address_Type;
    (void)Peer_Address[6];
    (void)Conn_Interval;
    (void)Conn_Latency;
    (void)Supervision_Timeout;
    (void)Master_Clock_Accuracy;

    app_state = APP_STATS_CONNECTED;
    connection_handle = Connection_Handle;
    memcpy(connectedPeerAddress, Peer_Address, 6);
}

void hci_le_enhanced_connection_complete_event(uint8_t Status,
                                               uint16_t Connection_Handle,
                                               uint8_t Role,
                                               uint8_t Peer_Address_Type,
                                               uint8_t Peer_Address[6],
                                               uint8_t Local_Resolvable_Private_Address[6],
                                               uint8_t Peer_Resolvable_Private_Address[6],
                                               uint16_t Conn_Interval,
                                               uint16_t Conn_Latency,
                                               uint16_t Supervision_Timeout,
                                               uint8_t Master_Clock_Accuracy)
{
    (void)Status;
    (void)Role;
    (void)Peer_Address_Type;
    (void)Local_Resolvable_Private_Address;
    (void)Peer_Resolvable_Private_Address;
    (void)Conn_Interval;
    (void)Conn_Latency;
    (void)Supervision_Timeout;
    (void)Master_Clock_Accuracy;

    app_state = APP_STATS_CONNECTED;
    connection_handle = Connection_Handle;
    memcpy(connectedPeerAddress, Peer_Address, 6);
}

void aci_gap_pairing_complete_event(uint16_t Connection_Handle,
                                    uint8_t Status,
                                    uint8_t Reason)
{
    (void)Status;
    (void)Connection_Handle;
    (void)Reason;
    // boardLedToggle(LED1);
}

void aci_gap_numeric_comparison_value_event(uint16_t Connection_Handle,
                                            uint32_t Numeric_Value)
{
    (void)Connection_Handle;
    aci_gap_numeric_comparison_value_confirm_yesno(Connection_Handle, 0x01);

    uart_printf("pin:%u\r\n", Numeric_Value);
}

void hci_disconnection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Reason)
{
    (void)Status;
    (void)Connection_Handle;
    (void)Reason;

    connection_handle = 0;
    app_state = APP_STATS_DISCONNECTED;
}

const char *ble_statusToStr(tBleStatus status)
{
    switch (status)
    {
    // Standard Error Codes
    case BLE_STATUS_SUCCESS:
        return "success ";
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
