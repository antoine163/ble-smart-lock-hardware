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
#include "bluenrg1_events.h"

// Implemented functions -------------------------------------------------------



#if 0

void hci_encryption_change_event(uint8_t Status,
                                 uint16_t Connection_Handle,
                                 uint8_t Encryption_Enabled)
{
//    boardLedToggle(LED1);
}





























void hci_read_remote_version_information_complete_event(uint8_t Status,
                                                        uint16_t Connection_Handle,
                                                        uint8_t Version,
                                                        uint16_t Manufacturer_Name,
                                                        uint16_t Subversion)
{
    boardLedToggle(LED1);
}


















void hci_hardware_error_event(uint8_t Hardware_Code)
{
    boardLedToggle(LED1);
}





















void hci_number_of_completed_packets_event(uint8_t Number_of_Handles,
                                           Handle_Packets_Pair_Entry_t Handle_Packets_Pair_Entry[])
{
    boardLedToggle(LED1);
}










void hci_data_buffer_overflow_event(uint8_t Link_Type)
{
    boardLedToggle(LED1);
}



















void hci_encryption_key_refresh_complete_event(uint8_t Status,
                                               uint16_t Connection_Handle)
{
    boardLedToggle(LED1);
}













tBleStatus hci_rx_acl_data_event(uint16_t Connection_Handle, uint8_t  PB_Flag, uint8_t  BC_Flag, uint16_t  Data_Length, uint8_t*  PDU_Data)
{
    boardLedToggle(LED1);
}

void hci_le_advertising_report_event(uint8_t Num_Reports,
                                     Advertising_Report_t Advertising_Report[])
{
    boardLedToggle(LED1);
}

void hci_le_connection_update_complete_event(uint8_t Status,
                                             uint16_t Connection_Handle,
                                             uint16_t Conn_Interval,
                                             uint16_t Conn_Latency,
                                             uint16_t Supervision_Timeout)
{
    boardLedToggle(LED1);
}

void hci_le_read_remote_used_features_complete_event(uint8_t Status,
                                                     uint16_t Connection_Handle,
                                                     uint8_t LE_Features[8])
{
    boardLedToggle(LED1);
}












void hci_le_long_term_key_request_event(uint16_t Connection_Handle,
                                        uint8_t Random_Number[8],
                                        uint16_t Encrypted_Diversifier)
{
    boardLedToggle(LED1);
}





























void hci_le_data_length_change_event(uint16_t Connection_Handle,
                                     uint16_t MaxTxOctets,
                                     uint16_t MaxTxTime,
                                     uint16_t MaxRxOctets,
                                     uint16_t MaxRxTime)
{
    boardLedToggle(LED1);
}







void hci_le_read_local_p256_public_key_complete_event(uint8_t Status,
                                                      uint8_t Local_P256_Public_Key[64])
{
    boardLedToggle(LED1);
}








void hci_le_generate_dhkey_complete_event(uint8_t Status,
                                          uint8_t DHKey[32])
{
    boardLedToggle(LED1);
}



void hci_le_direct_advertising_report_event(uint8_t Num_Reports,
                                            Direct_Advertising_Report_t Direct_Advertising_Report[])
{
    boardLedToggle(LED1);
}



void aci_gap_limited_discoverable_event(void)
{
    boardLedToggle(LED1);
}













void aci_gap_slave_security_initiated_event(void)
{
    boardLedToggle(LED1);
}













void aci_gap_bond_lost_event(void)
{
    boardLedToggle(LED1);
}





















void aci_gap_proc_complete_event(uint8_t Procedure_Code,
                                 uint8_t Status,
                                 uint8_t Data_Length,
                                 uint8_t Data[])
{
    boardLedToggle(LED1);
}









void aci_gap_addr_not_resolved_event(uint16_t Connection_Handle)
{
    boardLedToggle(LED1);
}



























void aci_gap_keypress_notification_event(uint16_t Connection_Handle,
                                         uint8_t Notification_Type)
{
    boardLedToggle(LED1);
}

void aci_gatt_proc_timeout_event(uint16_t Connection_Handle)
{
    boardLedToggle(LED1);
}




void aci_att_exchange_mtu_resp_event(uint16_t Connection_Handle,
                                     uint16_t Server_RX_MTU)
{
    boardLedToggle(LED1);
}



void aci_att_find_info_resp_event(uint16_t Connection_Handle,
                                  uint8_t Format,
                                  uint8_t Event_Data_Length,
                                  uint8_t Handle_UUID_Pair[])
{
    boardLedToggle(LED1);
}


void aci_att_find_by_type_value_resp_event(uint16_t Connection_Handle,
                                           uint8_t Num_of_Handle_Pair,
                                           Attribute_Group_Handle_Pair_t Attribute_Group_Handle_Pair[])
{
    boardLedToggle(LED1);
}


void aci_att_read_by_type_resp_event(uint16_t Connection_Handle,
                                     uint8_t Handle_Value_Pair_Length,
                                     uint8_t Data_Length,
                                     uint8_t Handle_Value_Pair_Data[])
{
    boardLedToggle(LED1);
}




void aci_att_read_resp_event(uint16_t Connection_Handle,
                             uint8_t Event_Data_Length,
                             uint8_t Attribute_Value[])
{
    boardLedToggle(LED1);
}








void aci_att_read_blob_resp_event(uint16_t Connection_Handle,
                                  uint8_t Event_Data_Length,
                                  uint8_t Attribute_Value[])
{
    boardLedToggle(LED1);
}









void aci_att_read_multiple_resp_event(uint16_t Connection_Handle,
                                      uint8_t Event_Data_Length,
                                      uint8_t Set_Of_Values[])
{
    boardLedToggle(LED1);
}












void aci_att_read_by_group_type_resp_event(uint16_t Connection_Handle,
                                           uint8_t Attribute_Data_Length,
                                           uint8_t Data_Length,
                                           uint8_t Attribute_Data_List[])
{
    boardLedToggle(LED1);
}










void aci_att_prepare_write_resp_event(uint16_t Connection_Handle,
                                      uint16_t Attribute_Handle,
                                      uint16_t Offset,
                                      uint8_t Part_Attribute_Value_Length,
                                      uint8_t Part_Attribute_Value[])
{
    boardLedToggle(LED1);
}





void aci_att_exec_write_resp_event(uint16_t Connection_Handle)
{
    boardLedToggle(LED1);
}









void aci_gatt_indication_event(uint16_t Connection_Handle,
                               uint16_t Attribute_Handle,
                               uint8_t Attribute_Value_Length,
                               uint8_t Attribute_Value[])
{
    boardLedToggle(LED1);
}









void aci_gatt_notification_event(uint16_t Connection_Handle,
                                 uint16_t Attribute_Handle,
                                 uint8_t Attribute_Value_Length,
                                 uint8_t Attribute_Value[])
{
    boardLedToggle(LED1);
}








void aci_gatt_proc_complete_event(uint16_t Connection_Handle,
                                  uint8_t Error_Code)
{
    boardLedToggle(LED1);
}
































void aci_gatt_error_resp_event(uint16_t Connection_Handle,
                               uint8_t Req_Opcode,
                               uint16_t Attribute_Handle,
                               uint8_t Error_Code)
{
    boardLedToggle(LED1);
}


















void aci_gatt_disc_read_char_by_uuid_resp_event(uint16_t Connection_Handle,
                                                uint16_t Attribute_Handle,
                                                uint8_t Attribute_Value_Length,
                                                uint8_t Attribute_Value[])
{
    boardLedToggle(LED1);
}






















void aci_gatt_write_permit_req_event(uint16_t Connection_Handle,
                                     uint16_t Attribute_Handle,
                                     uint8_t Data_Length,
                                     uint8_t Data[])
{
    boardLedToggle(LED1);
}














void aci_gatt_read_permit_req_event(uint16_t Connection_Handle,
                                    uint16_t Attribute_Handle,
                                    uint16_t Offset)
{
    boardLedToggle(LED1);
}















void aci_gatt_read_multi_permit_req_event(uint16_t Connection_Handle,
                                          uint8_t Number_of_Handles,
                                          Handle_Item_t Handle_Item[])
{
    boardLedToggle(LED1);
}











void aci_gatt_tx_pool_available_event(uint16_t Connection_Handle,
                                      uint16_t Available_Buffers)
{
    boardLedToggle(LED1);
}






void aci_gatt_server_confirmation_event(uint16_t Connection_Handle)
{
    boardLedToggle(LED1);
}




















void aci_gatt_prepare_write_permit_req_event(uint16_t Connection_Handle,
                                             uint16_t Attribute_Handle,
                                             uint16_t Offset,
                                             uint8_t Data_Length,
                                             uint8_t Data[])
{
    boardLedToggle(LED1);
}

















void aci_l2cap_connection_update_resp_event(uint16_t Connection_Handle,
                                            uint16_t Result)
{
    boardLedToggle(LED1);
}










void aci_l2cap_proc_timeout_event(uint16_t Connection_Handle,
                                  uint8_t Data_Length,
                                  uint8_t Data[])
{
    boardLedToggle(LED1);
}






























void aci_l2cap_connection_update_req_event(uint16_t Connection_Handle,
                                           uint8_t Identifier,
                                           uint16_t L2CAP_Length,
                                           uint16_t Interval_Min,
                                           uint16_t Interval_Max,
                                           uint16_t Slave_Latency,
                                           uint16_t Timeout_Multiplier)
{
    boardLedToggle(LED1);
}












void aci_l2cap_command_reject_event(uint16_t Connection_Handle,
                                    uint8_t Identifier,
                                    uint16_t Reason,
                                    uint8_t Data_Length,
                                    uint8_t Data[])
{
    boardLedToggle(LED1);
}















































void aci_hal_end_of_radio_activity_event(uint8_t Last_State,
                                         uint8_t Next_State,
                                         uint32_t Next_State_SysTime)
{
    boardLedToggle(LED1);
}




















void aci_hal_scan_req_report_event(int8_t RSSI,
                                   uint8_t Peer_Address_Type,
                                   uint8_t Peer_Address[6])
{
    boardLedToggle(LED1);
}















void aci_hal_fw_error_event(uint8_t FW_Error_Type,
                            uint8_t Data_Length,
                            uint8_t Data[])
{
    boardLedToggle(LED1);
}



#endif