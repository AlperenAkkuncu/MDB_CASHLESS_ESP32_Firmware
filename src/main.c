/* UART asynchronous example, that uses separate RX and TX tasks
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "../lib/MDB/MDB_core.h"
#include "../lib/USB/USB_com.h"



#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

#define LED_PIN 27

//uart_parity_t parity_mode = {UART_PARITY_EVEN};
void send_JUST_REST(uint8_t iteration);


void init_logging_uart(void){

    const uart_config_t uart_config = {
        .baud_rate = 230400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
        
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

/*
static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "Hello world");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
*/
void send_JUST_REST(uint8_t iteration){
    uint8_t TX_DATA[2], i;
    
    TX_DATA[0] = 0x00;

    for(i=0;i<iteration;i++){

        uart_set_parity(UART_NUM_1, UART_PARITY_EVEN);
        uart_write_bytes(UART_NUM_1,TX_DATA,1);
        uart_wait_tx_done(UART_NUM_1,  20 / portTICK_PERIOD_MS);
        uart_set_parity(UART_NUM_1, UART_PARITY_ODD);
        uart_write_bytes(UART_NUM_1,TX_DATA,1);
        uart_wait_tx_done(UART_NUM_1,  20 / portTICK_PERIOD_MS);

    }
        
}

void led_blink(void *pvParams) {
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction (LED_PIN,GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(LED_PIN,0);
        vTaskDelay(500/portTICK_RATE_MS);
        gpio_set_level(LED_PIN,1);
        vTaskDelay(500/portTICK_RATE_MS);
    }
}

void print_time_task(void *arg){
    static const char *PRINT_TIME_TAG = "PRINT_TIME_TASK";
    esp_log_level_set(PRINT_TIME_TAG, ESP_LOG_INFO);
    uint64_t start = 0;
    uint64_t end = esp_timer_get_time();
    while(1){
        end = esp_timer_get_time();
        ESP_LOGI(PRINT_TIME_TAG, "time: %llu", end-start);
        start = esp_timer_get_time();
        vTaskDelay(100/portTICK_RATE_MS);
    }
}

void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    char data[64];

    int length = 0;

    while (1) {
        
        
        
        uart_get_buffered_data_len(UART_NUM_0, (size_t*)&length);
        
        //ESP_LOGI(RX_TASK_TAG, "Cached bytes: %d", length);
        if(length>1){
            
            length = uart_read_bytes(UART_NUM_0, data, length, 10 / portTICK_PERIOD_MS);
            data[length] = '\0'; //null terminate
            if( memcmp(data, MDB_USB_BEGIN_SESSION_HEADER, strlen(MDB_USB_BEGIN_SESSION_HEADER))  == 0)
            {
                //data+strlen(MDB_USB_BEGIN_SESSION_HEADER);
                //ESP_LOGI(RX_TASK_TAG, "Num bytes: %d", length);
                uint16_t fund;
                fund = atoi(data+strlen(MDB_USB_BEGIN_SESSION_HEADER));
                if(fund>0)
                    MDB_send_fund(fund);       
                //ESP_LOGI(RX_TASK_TAG, "%d", ( atoi(data+strlen(MDB_USB_BEGIN_SESSION_HEADER)) ) );
            }

            
            
            ESP_LOGI(RX_TASK_TAG, "%s", data);
            length = 0;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);

    }
    free(data);
}

void app_main(void)
{   
    
    init_logging_uart();

    xTaskCreate(&rx_task, "uart_rx_task", 4096, NULL, 5, NULL);
    xTaskCreate(&MDB_core_task, "MDB_task", 4*1024,NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(&led_blink,"LED_BLINK",4*1024,NULL,4,NULL);
    //xTaskCreate(&print_time_task,"PRINT_TIME_TASK",1024*4,NULL,3,NULL);
    //xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}