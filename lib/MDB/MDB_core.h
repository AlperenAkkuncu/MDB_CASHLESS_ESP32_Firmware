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
#define CAHSLESS_READER_LEN 3

//vend packages lenght (including checksum)
#define CASHLESS_VEND_REQUEST_LEN 7
#define CASHLESS_VEND_CANCEL_LEN 3
#define CASHLESS_VEND_SUCCESS_LEN 5
#define CASHLESS_VEND_FAIL_LEN 3
#define CASHLESS_VEND_SESSION_COMPLETE_LEN 3
#define CASHLESS_VEND_CASH_SALE_LEN 7


//SETUP sub commands
#define SETUP_CONFIG  0x00
#define SETUP_MAX_MIN 0x01


//READER sub-commands
#define READER_DISABLE 0x00
#define READER_ENABLE 0x01
#define READER_CANCEL 0x02      //NOT IMPLEMENTED
#define READER_DATA_ENTRY 0x03  //NOT IMPLEMENTED


//VEND sub-commands
#define VEND_REQUEST 0x00
#define VEND_CANCEL 0x01
#define VEND_SUCCESS 0x02
#define VEND_FAILURE 0x03
#define VEND_SESSION_COMPLETE 0x04
#define VEND_CASH_SALE 0x05

//POLL RESPONSES
#define POLL_BEGIN_SESSION 0x03
#define POLL_VEND_APPROVED 0x05
#define POLL_VEND_DENIED 0x06
#define POLL_VEND_END_SESSION 0x07


enum E_parser_result{
    INCOMPLETE,
    INVALID,
    RESET_CMD,
    POLL_CMD,
    SETUP_CONFIG_CMD,
    SETUP_MAX_MIN_CMD,
    EXPANSINON_CMD,
    READER_CMD,
    VEND_REQUEST_CMD,
    VEND_CANCEL_CMD,
    VEND_SUCCESS_CMD,
    VEND_FAILURE_CMD,
    VEND_SESSION_COMPLETE_CMD,
    VEND_CASH_SALE_CMD,
};

enum E_poll_response{
    ACK,
    JUST_RESET,
    BEGIN_SESSION,
    VEND_APPROVE,
    VEND_DENY,
    END_SESSION
};
enum E_reader_state{
    ENABLED,
    DISABLED
};

struct S_fund {
    uint16_t fund_amount;
    bool is_fund_new;   //to notify MDB core that new fund has arrived
    bool is_fund_sent;  //to check if it's sent
};

struct S_vend_request{
    uint16_t item_number;
    uint16_t item_price;
};

struct S_vend_flags {
    bool vending_in_prog;  //set to one begin session to end session
    bool session_begun;    //set when BEGIN SESSION is sent receveied, un set when end session received
    bool vend_requested;   //set when VEND_REQUEST received, unset when end session received.
    bool vend_approved;    //set when VEND APPROVED is sent, unset when end session is received
    bool vend_denied;      //set when VEND DENIED is sent, unset when end session is received
    bool vend_succeed;     //set when VEND_SUCCESS is received, unset when end session is received
    bool vend_failed;      //set when VEND_FAILURE command received, unset when end session received.
    bool session_completed;
};



void MDB_core_task(void *arg);
void MDB_send_fund(uint16_t fund_amount);

#endif