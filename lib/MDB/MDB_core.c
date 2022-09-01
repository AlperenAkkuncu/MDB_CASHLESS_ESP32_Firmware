
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"


#include "MDB_uart.h"
#include "MDB_core.h"
#include "USB_com.h"
//Queue for receving packages from USB
QueueHandle_t USB_queue;

//Set CASHLESS device tpye
//For CASHLESS 1 -> CASHLESS_TYPE = 1
//For CASHLESS 2 -> CASHLESS_TYPE = 2
static int CASHLESS_TYPE = 1;

//SETUP CONFIG ARRAY
uint8_t CONFIG_DATA_LVL_2[8] = {0x01,0x02,0x19,0x49,0x01,0x02,0x3B,0x0D};
//READER CONFIG PACKAGE, response to expansion command
uint8_t READER_CONFIG_DATA[30] = {0x09, 0x4E, 0x59, 0x58, 0x30, 0x30, 
0x30, 0x30, 0x30, 0x30, 0x31, 0x39, 0x35, 0x34, 0x36, 0x37, 0x44, 0x4D,
0x58, 0x20, 0x2D, 0x20, 0x32, 0x30, 0x31, 0x31, 0x20, 0x20, 0x01, 0x00};
//PAYMENT ID, begin session
uint8_t PAYMENT_ID[4] = {0x35, 0x34, 0x36, 0x37};
//PAYMENT TYPE, begin session
uint8_t PAYMENT_TYPE[3] = {0x00, 0x00, 0x00}; 

/*
Global Variables and global flags
*/
struct S_fund fund;



/*
Private functions
*/
enum E_parser_result MDB_parse_package(uint8_t *data, uint8_t data_len);
bool MDB_check_checksum(uint8_t *data, uint8_t data_len);
void MDB_send_ACK(void);
void MDB_send_JUST_RESET(void);
void MDB_send_package(uint8_t *buffer, uint8_t len);
void MDB_send_BEGIN_SESSION(uint16_t fund_amount);

void MDB_core_task(void *arg)
{
    static const char *MDB_TAG = "MDB";
    esp_log_level_set(MDB_TAG, ESP_LOG_INFO);

    int buffered_data_len = 0;
    uint8_t MDB_data[64]; //holds newly arrived bytes
    uint8_t MDB_package[64], *p_MDB_package;//holds package
    uint8_t MDB_package_counter = 0;

    /*
    Global Variables and global flags
    */
    fund.fund_amount = 0;
    fund.is_fund_new = 0;
    fund.is_fund_sent = 0;

    //Buffers for queue messages
    char USB_queue_buffer[64];

    //enumerations
    enum E_parser_result parser_result = INVALID;
    enum E_poll_response poll_response = JUST_RESET;
    enum E_reader_state reader_state = DISABLED;


    //flags

  
    //Initiazlize UART for MDB
    init_MDB_uart();

    //MDB core main task starts here
    while (1) {


        /*
        Check for messages 
        */
        //check USB queue
        if(fund.is_fund_new){
            ESP_LOGI(MDB_TAG, "Got fund %d", fund.fund_amount);   
            fund.is_fund_new = 0;
            poll_response = BEGIN_SESSION;
        }


        buffered_data_len = get_buffered_data_len();
        
        if(buffered_data_len > 0){
            get_MDB_data(MDB_data, buffered_data_len);


            //concatenate MDB_data to MDB_package
            uint8_t i;
            for(i=0;i<buffered_data_len;i++){
                *(MDB_package + MDB_package_counter) =  *(MDB_data + i);
                MDB_package_counter++;
            }
            buffered_data_len = 0; 

            parser_result = MDB_parse_package(MDB_package, MDB_package_counter);

            switch(parser_result){
                case INVALID:
                    MDB_package_counter = 0;
                    //ESP_LOGI(MDB_TAG, "INVALID"); 
                break;

                case INCOMPLETE:
                    //ESP_LOGI(MDB_TAG, "INCOMPLETE"); 
                break;

                case RESET_CMD:
                    MDB_send_ACK();
                    poll_response = JUST_RESET;

                    //clean-up
                    MDB_package_counter = 0;

                    //LOG
                    ESP_LOGI(MDB_TAG, "RESET"); 
                break;

                case POLL_CMD: ;

                    if(poll_response == ACK)
                        MDB_send_ACK();
                    else if(poll_response == JUST_RESET){
                        MDB_send_JUST_RESET();
                        poll_response = ACK;
                        //get ACK
                        uint8_t ACK;
                        ACK = get_ACK();
                        if(ACK != 0x00)
                            ESP_LOGI(MDB_TAG, "JUST RESET ACK not received");     
                    }
                    else if(poll_response == BEGIN_SESSION){
                        MDB_send_BEGIN_SESSION(fund.fund_amount);
                        poll_response = ACK;
                        fund.is_fund_new = 0;
                        //get ACK
                        uint8_t ACK;
                        ACK = get_ACK();
                        if(ACK != 0x00){
                            ESP_LOGI(MDB_TAG, "JUST RESET ACK not received"); 
                        }
                        else{
                            ESP_LOGI(MDB_TAG, "BEGIN SESSION ACK"); 
                        }
                        fund.is_fund_sent = 1;
                            
                    }

                    //clean-up           
                    MDB_package_counter = 0;

                    //LOG
                    ESP_LOGI(MDB_TAG, "VALID POLL"); 
                break;

                case(SETUP_CONFIG_CMD): {
                    
                    MDB_send_package(CONFIG_DATA_LVL_2,sizeof(CONFIG_DATA_LVL_2));
                    //get ACK
                    uint8_t ACK;
                    ACK = get_ACK();
                    
                    //clean-up           
                    MDB_package_counter = 0;

                    //LOG
                    if(ACK != 0x00)
                        ESP_LOGI(MDB_TAG, "JUST RESET ACK not received"); 
                    else
                        ESP_LOGI(MDB_TAG, "VALID SETUP CONFIG"); 
                break;
                }

                case(SETUP_MAX_MIN_CMD): {
                    
                    MDB_send_ACK();

                    //clean-up           
                    MDB_package_counter = 0;

                    //LOG
                    ESP_LOGI(MDB_TAG, "VALID SETUP MAX/MIN"); 
                break;
                }

                case(EXPANSINON_CMD): {

                    MDB_send_package(READER_CONFIG_DATA,sizeof(READER_CONFIG_DATA));
                    //get ACK
                    uint8_t ACK;
                    ACK = get_ACK();
                     
                    //clean-up           
                    MDB_package_counter = 0;

                    //LOG
                    if(ACK != 0x00)
                        ESP_LOGI(MDB_TAG, "EXPANSION ACK not received");
                    else
                        ESP_LOGI(MDB_TAG, "VALID EXPANSION COMMAND"); 

                break;
                }

                case(READER_CMD): {

                    MDB_send_ACK();

                    if(MDB_package[1] == READER_DISABLE)
                        reader_state = DISABLED;
                    else if(MDB_package[1] == READER_ENABLE)
                        reader_state = ENABLED;
                    else
                        reader_state = DISABLED;
                    
                    //clean-up           
                    MDB_package_counter = 0;

                    //LOG
                    if(reader_state == ENABLED)
                        ESP_LOGI(MDB_TAG, "READER ENABLED"); 
                    else
                        ESP_LOGI(MDB_TAG, "READER DISABLED"); 

                break;
                }


                default:
                    MDB_package_counter = 0;
            }


            

            //ESP_LOGI(MDB_TAG, "Cached bytes: %d", buffered_data_len);
            
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
  
    }
    free(MDB_data);
}

enum E_parser_result MDB_parse_package(uint8_t *data, uint8_t data_len){
    uint8_t *p_data = data;


    if(CASHLESS_TYPE == 1){
        if(*p_data == CASHLESS1_RESET_CMD      ||
           *p_data == CASHLESS1_SETUP_CMD      ||
           *p_data == CASHLESS1_POLL_CMD       ||
           *p_data == CASHLESS1_VEND_CMD       ||
           *p_data == CASHLESS1_READER_CMD     ||
           *p_data == CASHLESS1_REVALUE        ||
           *p_data == CASHLESS1_EXPANSION_CMD  )
        {
            uint8_t cmd = *p_data;
            uint8_t sub_cmd ;
            if(data_len >= 2)
                sub_cmd = *(p_data+1);
            

            switch (cmd)
            {   
                //PARSE RESET CMD START
                case CASHLESS1_RESET_CMD:
                    if(data_len<CASHLESS_RESET_LEN)
                        return INCOMPLETE;
                    else if(data_len == CASHLESS_RESET_LEN){ 
                        bool checksum_state;
                        checksum_state = MDB_check_checksum(data,data_len);
                        //check checksum
                        if(checksum_state)
                            return RESET_CMD;
                        else
                            return INVALID; 
                    }

                break;
                //PARSE RESET CMD END
                
                //PARSE POLL CMD START
                case CASHLESS1_POLL_CMD:
                    if(data_len<CASHLESS_POLL_LEN)
                        return INCOMPLETE;
                    else if(data_len == CASHLESS_POLL_LEN){
                        bool checksum_state;
                        checksum_state = MDB_check_checksum(data,data_len);
                        //check checksum
                        if(checksum_state)
                            return POLL_CMD;
                        else
                            return INVALID;
                    }
                    else 
                        return INVALID;
                break;
                //PARSE POLL CMD END

                //PARSE SETUP CMD START
                case CASHLESS1_SETUP_CMD:
                    if(data_len<CASHLESS_SETUP_CONFIG_LEN)
                        return INCOMPLETE;
                    else if(sub_cmd == SETUP_CONFIG && data_len == CASHLESS_SETUP_CONFIG_LEN){
                        bool checksum_state;
                        checksum_state = MDB_check_checksum(data,data_len);
                        //check checksum
                        if(checksum_state)
                            return SETUP_CONFIG_CMD;
                        else
                            return INVALID; 
                    }
                    else if(sub_cmd == SETUP_MAX_MIN && data_len == CASHLESS_SETUP_MAX_MIX_LEN){
                        bool checksum_state;
                        checksum_state = MDB_check_checksum(data,data_len);
                        //check checksum
                        if(checksum_state)
                            return SETUP_MAX_MIN_CMD;
                        else
                            return INVALID; 
                    }
                    else 
                        return INVALID;
                break;
                //PARSE SETUP CMD STOP

                //EXPANSION CMD START
                case(CASHLESS1_EXPANSION_CMD):
                    if(data_len<CASHLESS_EXPANSION_LEN)
                        return INCOMPLETE;
                    else if(data_len == CASHLESS_EXPANSION_LEN){
                        bool checksum_state;
                        checksum_state = MDB_check_checksum(data,data_len);
                        //check checksum
                        if(checksum_state)
                            return EXPANSINON_CMD;
                        else
                            return INVALID;
                    }
                    else 
                        return INVALID;
                break;
                //EXPANSION CMD STOP
            
                //READER CMD START
                case(CASHLESS1_READER_CMD):
                    if(data_len<CAHSLESS_READER_LEN)
                        return INCOMPLETE;
                    else if(data_len == CAHSLESS_READER_LEN){
                        bool checksum_state;
                        checksum_state = MDB_check_checksum(data,data_len);
                        //check checksum
                        if(checksum_state)
                            return READER_CMD;
                        else
                            return INVALID;
                    }
                    else 
                        return INVALID;
                break;
                //READER CMD END

                //PARSE VEND CMD START
                case CASHLESS1_VEND_CMD:
                    if(data_len<CASHLESS_VEND_CANCEL_LEN) //shorter than shortest package
                        return INCOMPLETE;

                    else if(sub_cmd == VEND_REQUEST){
                        if(data_len == CASHLESS_VEND_REQUEST_LEN){
                            bool checksum_state;
                            checksum_state = MDB_check_checksum(data,data_len);
                            //check checksum
                            if(checksum_state)
                                return VEND_REQUEST_CMD;
                            else
                                return INVALID; 
                        }
                        else if(data_len < CASHLESS_VEND_REQUEST_LEN)
                            return INCOMPLETE;
                        else 
                            return INVALID;
                    }

                    else if(sub_cmd == VEND_CANCEL){
                        if(data_len == CASHLESS_VEND_CANCEL_LEN){
                            bool checksum_state;
                            checksum_state = MDB_check_checksum(data,data_len);
                            //check checksum
                            if(checksum_state)
                                return VEND_CANCEL_CMD;
                            else
                                return INVALID; 
                        }
                        else if(data_len < CASHLESS_VEND_CANCEL_LEN)
                            return INCOMPLETE;
                        else 
                            return INVALID;
                    }

                    else if(sub_cmd == VEND_SUCCESS && data_len == CASHLESS_VEND_SUCCESS_LEN){
                        if(data_len == CASHLESS_VEND_SUCCESS_LEN){
                            bool checksum_state;
                            checksum_state = MDB_check_checksum(data,data_len);
                            //check checksum
                            if(checksum_state)
                                return VEND_SUCCESS_CMD;
                            else
                                return INVALID; 
                        }
                        else if(data_len < CASHLESS_VEND_SUCCESS_LEN)
                            return INCOMPLETE;
                        else 
                            return INVALID;
                    }
                    else if(sub_cmd == VEND_FAILURE){
                        if(data_len == CASHLESS_VEND_FAIL_LEN){
                            bool checksum_state;
                            checksum_state = MDB_check_checksum(data,data_len);
                            //check checksum
                            if(checksum_state)
                                return VEND_FAILURE_CMD;
                            else
                                return INVALID; 
                        }
                        else if(data_len < CASHLESS_VEND_FAIL_LEN)
                            return INCOMPLETE;
                        else 
                            return INVALID;
                    }
                    else if(sub_cmd == VEND_SESSION_COMPLETE){
                        if(data_len == CASHLESS_VEND_SESSION_COMPLETE_LEN) {  
                            bool checksum_state;
                            checksum_state = MDB_check_checksum(data,data_len);
                            //check checksum
                            if(checksum_state)
                                return VEND_SESSION_COMPLETE_CMD;
                            else
                                return INVALID; 
                        }
                        else if(data_len < CASHLESS_VEND_SESSION_COMPLETE_LEN)
                            return INCOMPLETE;
                        else 
                            return INVALID;
                        
                    }
                    else if(sub_cmd == VEND_CASH_SALE_CMD){
                        if(data_len == CASHLESS_VEND_CASH_SALE_LEN){
                            bool checksum_state;
                            checksum_state = MDB_check_checksum(data,data_len);
                            //check checksum
                            if(checksum_state)
                                return VEND_CASH_SALE_CMD;
                            else
                                return INVALID; 
                        }
                        else if(data_len < CASHLESS_VEND_CASH_SALE_LEN)
                            return INCOMPLETE;
                        else
                            return INVALID;
                    }
                    else 
                        return INVALID;
                break;
                //PARSE VEND CMD END


                default:
                    return INVALID;
            }
        }
        else //first byte is not valid (not cashless)
            return INVALID;
    }
    else if(CASHLESS_TYPE == 2){
        if(*p_data == CASHLESS2_RESET_CMD      ||
           *p_data == CASHLESS2_SETUP_CMD      ||
           *p_data == CASHLESS2_POLL_CMD       ||
           *p_data == CASHLESS2_VEND_CMD       ||
           *p_data == CASHLESS2_READER_CMD     ||
           *p_data == CASHLESS2_REVALUE        ||
           *p_data == CASHLESS2_EXPANSION_CMD )
        {
            return POLL_CMD;

        }
        else //first byte is not valid (not cashless)
            return INVALID;
    }
    return INVALID;
}

bool MDB_check_checksum(uint8_t *data, uint8_t data_len){
    uint8_t i, sum = 0,*p_data = data;

    for(i=0;i<(data_len-1);i++){
        sum = sum + *p_data;
        p_data++;
    }
    
    if(*p_data == sum)
        return true;
    else
        return false;

}

void MDB_send_ACK(void){
    send_byte_set_9th_bit(0x00);
}

void MDB_send_JUST_RESET(void){
    send_byte_unset_9th_bit(0x00);
    send_byte_set_9th_bit(0x00);
}

void MDB_send_package(uint8_t *buffer, uint8_t len){
    uint8_t i, checksum = 0, *p_buffer = buffer;

    for(i=0;i<len;i++){
        checksum = checksum + *p_buffer;
        p_buffer++;
    }
    
    p_buffer = buffer;
    for(i=0;i<len;i++){
        send_byte_unset_9th_bit(*p_buffer);
        p_buffer++;
    }
    //send checksum

    send_byte_set_9th_bit(checksum);

}

void MDB_send_BEGIN_SESSION(uint16_t fund_amount){
    uint8_t i=0, MDB_TX[12];

    MDB_TX[i++] = 0x03; // begin session header
    MDB_TX[i++] = (uint8_t)(fund_amount>>8);
    MDB_TX[i++] = (uint8_t) (fund_amount);

    uint8_t k;
    for(k=0;k<4;k++)
        MDB_TX[i++] = PAYMENT_ID[k]; 
    for(k=0;k<3;k++)
        MDB_TX[i++] = PAYMENT_TYPE[k]; 
    MDB_send_package(MDB_TX,10);
}


/****************************/
/***MDB _Core communication***/
/****************************/

//sends fund to the screen
void MDB_send_fund(uint16_t fund_amount){
    if(fund.is_fund_new == 0){
        fund.fund_amount = fund_amount;
        fund.is_fund_new = 1;
    }

}