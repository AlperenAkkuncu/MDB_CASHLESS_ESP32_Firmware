#ifndef MDB_CORE_H
#define MDB_CORE_H
//CAHSLESS devices commands

//CASHLESS 1
#define CASHLESS1_ADDR          0x10 
#define CASHLESS1_RESET_CMD     0x10
#define CASHLESS1_SETUP_CMD     0x11
#define CASHLESS1_POLL_CMD      0x12
#define CASHLESS1_VEND_CMD      0x13
#define CASHLESS1_READER_CMD    0x14
#define CASHLESS1_REVALUE       0x15
#define CASHLESS1_EXPANSION_CMD 0x17
    
//CASHLESS 2
#define CASHLESS2_ADDR          0x60 
#define CASHLESS2_RESET_CMD     0x60
#define CASHLESS2_SETUP_CMD     0x61
#define CASHLESS2_POLL_CMD      0x62
#define CASHLESS2_VEND_CMD      0x63
#define CASHLESS2_READER_CMD    0x64
#define CASHLESS2_REVALUE       0x65
#define CASHLESS2_EXPANSION_CMD 0x67

//package lengths (including checksum)

#define CASHLESS_POLL_LEN 2
#define CASHLESS_RESET_LEN 2
#define CASHLESS_SETUP_CONFIG_LEN 7
#define CASHLESS_SETUP_MAX_MIX_LEN 7
#define CASHLESS_EXPANSION_LEN 32


//SETUP sub commands
#define SETUP_CONFIG  0x00
#define SETUP_MAX_MIN 0x01



enum E_parser_result{
    INCOMPLETE,
    INVALID,
    VALID_RESET_CMD,
    VALID_POLL_CMD,
    VALID_SETUP_CONFIG_CMD,
    VALID_SETUP_MAX_MIN
};

enum E_poll_response{
    ACK,
    JUST_RESET
};

void MDB_core_task(void *arg);


#endif