/* FILE: uart_comm.h */

#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// REQ-MCU-001: Define the UART baud rate
#define UART_BAUD_RATE 115200

// REQ-MCU-002: Define UART configuration options
typedef enum {
    UART_PARITY_NONE,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
} UART_Parity;

typedef enum {
    UART_STOP_BITS_1,
    UART_STOP_BITS_2
} UART_StopBits;

typedef struct {
    UART_Parity parity;
    UART_StopBits stop_bits;
    uint8_t data_bits;
} UART_Config;

// REQ-MCU-003: Define UART status codes
typedef enum {
    UART_STATUS_OK,
    UART_STATUS_ERROR_PARAMETER,
    UART_STATUS_ERROR_PARITY,
    UART_STATUS_ERROR_FRAMING,
    UART_STATUS_ERROR_OVERFLOW,
    UART_STATUS_ERROR_TIMEOUT
} UART_Status;

// Function signatures

/**
 * REQ-MCU-004: Initialize the UART module
 * @param config UART configuration structure
 * @return Status code indicating initialization success or failure
 */
UART_Status UART_Init(const UART_Config *config);

/**
 * REQ-MCU-005: Transmit a single byte over UART
 * @param data Byte to be transmitted
 * @return Status code indicating transmission success or failure
 */
UART_Status UART_Transmit(uint8_t data);

/**
 * REQ-MCU-006: Receive a single byte over UART
 * @param p_data Pointer to store the received byte
 * @return Status code indicating reception success or failure
 */
UART_Status UART_Receive(uint8_t *p_data);

/**
 * REQ-MCU-007: Transmit a string over UART
 * @param str String to be transmitted
 * @return Status code indicating transmission success or failure
 */
UART_Status UART_TransmitString(const char *str);

/**
 * REQ-MCU-008: Receive a string from UART until a newline character is received
 * @param buffer Buffer to store the received string
 * @param max_length Maximum length of the string to be received
 * @return Status code indicating reception success or failure
 */
UART_Status UART_ReceiveString(char *buffer, size_t max_length);

extern uint32_t SystemCoreClock;

#endif // UART_COMM_H
