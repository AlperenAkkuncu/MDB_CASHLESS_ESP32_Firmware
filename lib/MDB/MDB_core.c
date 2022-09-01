
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"


#include "MDB_uart.h"
#include "MDB_core.h"



//Set CASHLESS device tpye
//For CASHLESS 1 -> CASHLESS_TYPE = 1
//For CASHLESS 2 -> CASHLESS_TYPE = 2
static int CASHLESS_TYPE = 1;

//SETUP CONFIG ARRAY
uint8_t CONFIG_DATA_LVL_2[8] = {0x01,0x02,0x19,0x49,0x01,0x02,0x3B,0x0D};

/*
Private functions
*/
enum E_parser_result MDB_parse_package(uint8_t *data, uint8_t data_len);
bool MDB_check_checksum(uint8_t *data, uint8_t data_len);
void MDB_send_ACK(void);
void MDB_send_JUST_RESET(void);
void MDB_send_package(uint8_t *buffer, uint8_t len);

void MDB_core_task(void *arg)
{
    static const char *MDB_TAG = "MDB";
    esp_log_level_set(MDB_TAG, ESP_LOG_INFO);

    int buffered_data_len = 0;
    uint8_t MDB_data[64]; //holds newly arrived bytes
    uint8_t MDB_package[64], *p_MDB_package;//holds package
    uint8_t MDB_package_counter = 0;


    //enumerations
    enum E_parser_result parser_result = INVALID;
    enum E_poll_response poll_response = JUST_RESET;
    


    //flags

  
    //Initiazlize UART for MDB
    init_MDB_uart();

    //MDB core main task starts here
    while (1) {
        
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
                    ESP_LOGI(MDB_TAG, "INVALID"); 
                break;

                case INCOMPLETE:
                    ESP_LOGI(MDB_TAG, "INCOMPLETE"); 
                break;

                case VALID_RESET_CMD:
                    MDB_send_ACK();
                    poll_response = JUST_RESET;

                    //clean-up
                    MDB_package_counter = 0;

                    //LOG
                    ESP_LOGI(MDB_TAG, "RESET"); 
                break;

                case VALID_POLL_CMD: ;

                    /*
                    uint8_t bitcount[256];
                    uint8_t i;

                    for(i=0;i<255;i++)
                        bitcount[i] = count_bits(i);
                    
                    for(i=0;i<255;i++)
                        ESP_LOGI(MDB_TAG, "num: %X - %d",i,bitcount[i]);     
                    */

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

                    //clean-up           
                    MDB_package_counter = 0;

                    //LOG
                    ESP_LOGI(MDB_TAG, "VALID POLL"); 
                break;

                case(VALID_SETUP_CONFIG_CMD):
                    
                    MDB_send_package(CONFIG_DATA_LVL_2,sizeof(CONFIG_DATA_LVL_2));
                    //get ACK
                    uint8_t ACK;
                    ACK = get_ACK();
                    if(ACK != 0x00)
                        ESP_LOGI(MDB_TAG, "JUST RESET ACK not received"); 
                        
                    //clean-up           
                    MDB_package_counter = 0;

                    //LOG
                    ESP_LOGI(MDB_TAG, "VALID SETUP CONFIG"); 
                break;

                case(VALID_SETUP_MAX_MIN):
                    
                    MDB_send_ACK();

                    //clean-up           
                    MDB_package_counter = 0;

                    //LOG
                    ESP_LOGI(MDB_TAG, "VALID SETUP MAX/MIN"); 
                break;


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
                            return VALID_RESET_CMD;
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
                            return VALID_POLL_CMD;
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
                            return VALID_SETUP_CONFIG_CMD;
                        else
                            return INVALID; 
                    }
                    else if(sub_cmd == SETUP_MAX_MIN && data_len == CASHLESS_SETUP_MAX_MIX_LEN){
                        bool checksum_state;
                        checksum_state = MDB_check_checksum(data,data_len);
                        //check checksum
                        if(checksum_state)
                            return VALID_SETUP_MAX_MIN;
                        else
                            return INVALID; 
                    }
                    else 
                        return INVALID;
                break;
                //PARSE SETUP CMD STOP
            

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
            return VALID_POLL_CMD;

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