/* FILE: uart_comm.c */

#include "uart_comm.h"

// Hypothetical base addresses for UART registers
#ifndef UART_BASE
#define UART_BASE 0x4000C000
#endif

#define UART_THR (*(volatile uint32_t *)(UART_BASE + 0x00)) // Transmitter Holding Register
#define UART_RBR (*(volatile uint32_t *)(UART_BASE + 0x00)) // Receiver Buffer Register
#define UART_LCR (*(volatile uint32_t *)(UART_BASE + 0x0C)) // Line Control Register
#define UART_LSR (*(volatile uint32_t *)(UART_BASE + 0x14)) // Line Status Register
#define UART_UBR (*(volatile uint32_t *)(UART_BASE + 0x2C)) // UART Baud Rate Register

// UART status masks
#define UART_LSR_THRE  0x20 // Transmit Holding Register Empty
#define UART_LSR_DR    0x01 // Data Ready
#define UART_LSR_FE    0x02 // Framing Error
#define UART_LSR_PE    0x04 // Parity Error
#define UART_LSR_OE    0x08 // Overrun Error

// Function implementations

UART_Status UART_Init(const UART_Config *config) {
    // REQ-MCU-004 Implementation: Initialize the UART module with the given configuration
    uint32_t lcr = 0;

    // Configure data bits
    if (config->data_bits == 8) {
        lcr |= 0x03; // 8 data bits
    } else if (config->data_bits == 7) {
        lcr |= 0x02; // 7 data bits
    } else if (config->data_bits == 6) {
        lcr |= 0x01; // 6 data bits
    } else {
        return UART_STATUS_ERROR_PARAMETER;
    }

    // Configure stop bits
    if (config->stop_bits == UART_STOP_BITS_1) {
        lcr |= 0x00; // 1 stop bit
    } else if (config->stop_bits == UART_STOP_BITS_2) {
        lcr |= 0x04; // 2 stop bits
    } else {
        return UART_STATUS_ERROR_PARAMETER;
    }

    // Configure parity
    switch (config->parity) {
        case UART_PARITY_NONE:
            lcr |= 0x00; // No parity
            break;
        case UART_PARITY_EVEN:
            lcr |= 0x18; // Even parity
            break;
        case UART_PARITY_ODD:
            lcr |= 0x08; // Odd parity
            break;
        default:
            return UART_STATUS_ERROR_PARAMETER;
    }

    UART_LCR = lcr;

    // Calculate baud rate divisor
    uint32_t baud_divisor = (SystemCoreClock / (16 * UART_BAUD_RATE)) - 1;
    UART_UBR = baud_divisor;

    return UART_STATUS_OK;
}

UART_Status UART_Transmit(uint8_t data) {
    // REQ-MCU-005 Implementation: Transmit a single byte of data over UART
    if ((UART_LSR & UART_LSR_THRE) == 0) {
        return UART_STATUS_ERROR_TIMEOUT;
    }
    UART_THR = data;
    return UART_STATUS_OK;
}

UART_Status UART_Receive(uint8_t *p_data) {
    // REQ-MCU-006 Implementation: Receive a single byte of data from UART
    if ((UART_LSR & UART_LSR_DR) == 0) {
        return UART_STATUS_ERROR_TIMEOUT;
    }
    *p_data = UART_RBR;

    if (UART_LSR & UART_LSR_FE) {
        return UART_STATUS_ERROR_FRAMING;
    }
    if (UART_LSR & UART_LSR_PE) {
        return UART_STATUS_ERROR_PARITY;
    }
    if (UART_LSR & UART_LSR_OE) {
        return UART_STATUS_ERROR_OVERFLOW;
    }

    return UART_STATUS_OK;
}

UART_Status UART_TransmitString(const char *str) {
    // REQ-MCU-007 Implementation: Transmit a string over UART
    UART_Status status;
    while (*str) {
        status = UART_Transmit(*str++);
        if (status != UART_STATUS_OK) {
            return status;
        }
    }
    return UART_STATUS_OK;
}

UART_Status UART_ReceiveString(char *buffer, size_t max_length) {
    // REQ-MCU-008 Implementation: Receive a string until a newline character is received or max_length is reached
    UART_Status status;
    size_t index = 0;
    uint8_t ch;
    while (index < max_length - 1) {
        status = UART_Receive(&ch);
        if (status != UART_STATUS_OK) {
            return status;
        }
        buffer[index++] = ch;
        if (ch == '\n') {
            break;
        }
    }
    buffer[index] = '\0'; // Null-terminate the string
    return UART_STATUS_OK;
}
