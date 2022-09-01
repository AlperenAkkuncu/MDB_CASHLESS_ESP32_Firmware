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
static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

#define LED_PIN 27

//uart_parity_t parity_mode = {UART_PARITY_EVEN};
void send_JUST_REST(uint8_t iteration);

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "Hello world");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

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
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    uint8_t TX_DATA[2];
    TX_DATA[0] = 0x00;
    while (1) {
        
        
        int length = 0;
        uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length);
        
        //ESP_LOGI(RX_TASK_TAG, "Cached bytes: %d", length);
        if(length==2){
            uint8_t iter = 200;
            uint64_t start = esp_timer_get_time();

            length = uart_read_bytes(UART_NUM_1, data, length, 1 / portTICK_PERIOD_MS);   

            
            send_JUST_REST(iter);

            uint64_t end = esp_timer_get_time();
            length = uart_read_bytes(UART_NUM_1, data, iter, 100 / portTICK_PERIOD_MS);
            length = 0;

            
            
            ESP_LOGI(RX_TASK_TAG, "Time: %llu", end-start);
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);

    }
    free(data);
}

void app_main(void)
{   
    uart_set_baudrate(UART_NUM_0, 230400); //faster baud rate.
    //xTaskCreate(&rx_task, "uart_rx_task", 4096, NULL, 5, NULL);
    xTaskCreate(&MDB_core_task, "MDB_task", 4*1024,NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(&led_blink,"LED_BLINK",4*1024,NULL,4,NULL);
    //xTaskCreate(&print_time_task,"PRINT_TIME_TASK",1024*4,NULL,3,NULL);
    //xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}