#include <stdio.h>

#include "MDB_uart.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "hal/uart_hal.h"
#include "soc/uart_struct.h"
#include "soc/soc.h"
#include "esp_log.h"
#include "sdkconfig.h"


#define MDB_UART UART_NUM_1

#define TEST_TOUT_SYMB          (5) 
#define TEST_RETRIES            (15)
#define TEST_BUF_SIZE           (256)

#define RXFIFO_FULL_THRD        (15)        // Set threshold for RX_FULL interrupt

#define MDB_TXD  (GPIO_NUM_17)
#define MDB_RXD  (GPIO_NUM_16)

#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE) //not used
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE) //not used

/*
Private functions
*/


uart_parity_t parity_mode = {UART_PARITY_EVEN};


void init_MDB_uart(void){

    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_ODD,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
        
    };
    // We won't use a buffer for sending data.
    uart_set_parity(MDB_UART, parity_mode);
    uart_driver_install(MDB_UART, 1024, 0, 0, NULL, 0);
    uart_param_config(MDB_UART, &uart_config);
    uart_set_pin(MDB_UART, MDB_TXD, MDB_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_line_inverse(MDB_UART,UART_SIGNAL_TXD_INV); //invert TX signal 
    uart_set_tx_idle_num(MDB_UART, 1);
    uart_set_rx_full_threshold(MDB_UART, 1);

}


uint8_t count_bits(uint8_t byte){
    //count
    uint8_t i = 0, setbit_count = 0;
    uint8_t byte_buf;
    byte_buf = byte;
    for(i=0;i<8;i++){
        setbit_count = setbit_count + (byte_buf & 1);
        byte_buf = byte_buf >>1;
    }
    return setbit_count;
}

int get_buffered_data_len(void){
    int length = 0;

    uart_get_buffered_data_len(MDB_UART, (size_t*)&length);
    return length;
}

void get_MDB_data(uint8_t *buffer, int len){
    int length = 0;

    length = uart_read_bytes(MDB_UART, buffer, len, 1 / portTICK_PERIOD_MS);
}

uint8_t get_ACK(void){
    int length = 0;
    uint8_t data;

    length = uart_read_bytes(MDB_UART, &data, 1, 10 / portTICK_PERIOD_MS);

    if(length == 0)
        return 0xFF;
    else
        return data;

}

void send_byte_set_9th_bit(uint8_t byte){
    uint8_t num_bits;
    
    num_bits = count_bits(byte);

    if( (num_bits % 2) == 0){
        uart_set_parity(UART_NUM_1, UART_PARITY_ODD);
        uart_write_bytes(UART_NUM_1,&byte,1);
        uart_wait_tx_done(UART_NUM_1,  20 / portTICK_PERIOD_MS);
    }
    else{
        uart_set_parity(UART_NUM_1, UART_PARITY_EVEN);
        uart_write_bytes(UART_NUM_1,&byte,1);
        uart_wait_tx_done(UART_NUM_1,  20 / portTICK_PERIOD_MS);
    }


}


void send_byte_unset_9th_bit(uint8_t byte){

        uint8_t num_bits;
    
    num_bits = count_bits(byte);

    if( (num_bits % 2) == 0){
        uart_set_parity(UART_NUM_1, UART_PARITY_EVEN);
        uart_write_bytes(UART_NUM_1,&byte,1);
        uart_wait_tx_done(UART_NUM_1,  20 / portTICK_PERIOD_MS);
    }
    else{
        uart_set_parity(UART_NUM_1, UART_PARITY_ODD);
        uart_write_bytes(UART_NUM_1,&byte,1);
        uart_wait_tx_done(UART_NUM_1,  20 / portTICK_PERIOD_MS);
    }

    
}
