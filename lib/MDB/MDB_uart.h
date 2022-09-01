#ifndef MDB_UART_H
#define MDB_UART_H

void init_MDB_uart(void);
int get_buffered_data_len(void);
void get_MDB_data(uint8_t *buffer, int len);
uint8_t get_ACK(void);
void send_byte_set_9th_bit(uint8_t byte);
void send_byte_unset_9th_bit(uint8_t byte);
uint8_t count_bits(uint8_t byte);
#endif
