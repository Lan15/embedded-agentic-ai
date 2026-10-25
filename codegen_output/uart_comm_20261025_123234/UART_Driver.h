#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    UART_ERROR_NONE,
    UART_ERROR_FRAMING,
    UART_ERROR_NOISE,
    UART_ERROR_OVERRUN
} UART_Error_t;
typedef enum {
    UART_STATE_IDLE,
    UART_STATE_RECEIVING,
    UART_STATE_TRANSMITTING
} UART_State_t;

extern UART_State_t uart_state;
extern UART_Error_t uart_error_status;
extern uint32_t SystemCoreClock;

UART_Error_t UART_Init(uint32_t baud_rate);
UART_Error_t UART_TransmitByte(uint8_t data);
UART_Error_t UART_ReceiveByte(uint8_t *ptr_data);
UART_Error_t UART_GetErrorStatus(void);
void UART_ClearErrors(UART_Error_t error_mask);

#endif
