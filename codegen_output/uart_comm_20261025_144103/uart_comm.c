/* FILE: uart_comm.c */
#include "uart_comm.h"

#define UART_BASE 0x4000C000  // Example base address for UART

// Register addresses
#define UART_THR (volatile uint32_t *)(UART_BASE + 0x00)  // Transmit Holding Register
#define UART_RBR (volatile uint32_t *)(UART_BASE + 0x00)  // Receive Buffer Register
#define UART_LSR (volatile uint32_t *)(UART_BASE + 0x14)  // Line Status Register

// Line Status Register bits
#define UART_LSR_THRE (1 << 5)  // Transmitter Holding Register Empty
#define UART_LSR_DR   (1 << 0)  // Data Ready

extern uint32_t SystemCoreClock;

UART_Error_t last_error = UART_NO_ERROR;

UART_Error_t UART_Init(const UART_Config_t *config) {
    // Example initialization code
    if (config->baud_rate != 115200) {
        last_error = UART_CONFIG_ERROR;
        return last_error;
    }
    // Additional initialization code for parity, stop bits, and flow control
    return UART_NO_ERROR;
}

// Function to transmit data over UART
UART_Error_t UART_Transmit(const uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        while ((*UART_LSR & UART_LSR_THRE) == 0) {
            // Wait until the transmitter is empty
        }
        *UART_THR = data[i];
    }
    return UART_NO_ERROR;
}

// Function to receive data over UART
UART_Error_t UART_Receive(uint8_t *buffer, uint16_t buffer_size, uint16_t *received_length) {
    *received_length = 0;
    while (*received_length < buffer_size) {
        if ((*UART_LSR & UART_LSR_DR) != 0) {
            buffer[*received_length] = *UART_RBR;
            (*received_length)++;
        } else {
            break;
        }
    }
    return UART_NO_ERROR;
}

// Function to get the last UART error
UART_Error_t UART_GetError(void) {
    return last_error;
}
