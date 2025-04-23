
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     2000000
#define TXD_PIN            (GPIO_NUM_11)
#define RXD_PIN            (GPIO_NUM_12)
#define BUF_SIZE           4096

typedef struct {
    uint8_t start[2];      
    uint32_t id;           
    int64_t time_delta;    
    float cir[4];          
    uint8_t end[2];        
} __attribute__((packed)) csi_data_t;

void init_uart(void);
int uart_send_data(const char *data, uint8_t len);

#ifdef __cplusplus
}
#endif