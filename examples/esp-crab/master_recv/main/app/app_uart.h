
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t start[2];      
    uint32_t id;           
    int64_t time_delta;    
    float cir[4];          
    uint8_t end[2];        
} __attribute__((packed)) csi_data_t;

#define DATA_TABLE_SIZE                     100
void init_uart(void);
int uart_send_data(const char *data, uint8_t len);

#ifdef __cplusplus
}
#endif