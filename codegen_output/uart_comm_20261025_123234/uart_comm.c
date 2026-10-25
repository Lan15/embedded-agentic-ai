// UART_Driver.c
#include "UART_Driver.h"
#include <stdint.h>

// Module Name: UART_Driver

// Register Base Address Simulation
#define UART_BASE 0x4000C000

// UART Register Offsets
#define UART_CR1_OFFSET 0x00000000
#define UART_DR_OFFSET  0x00000004
#define UART_SR_OFFSET  0x0000000C
#define UART_BRR_OFFSET 0x00000008

// UART Control Register Bits
#define UART_CR1_TE 0x0080
#define UART_CR1_RE 0x0040

// UART Status Register Bits
#define UART_SR_TXE  0x0080
#define UART_SR_RXNE 0x0020
#define UART_SR_ORE  0x0020
#define UART_SR_FE   0x0010
#define UART_SR_NE   0x0008

// UART Baud Rate Register
#define UART_BRR_VALUE(baud_rate) (SystemCoreClock / (16 * baud_rate))

// Internal State
UART_State_t uart_state;
UART_Error_t uart_error_status;

// Function Implementations

UART_Error_t UART_Init(uint32_t baud_rate)
{
    volatile uint32_t *uart_cr1 = (volatile uint32_t *)(UART_BASE + UART_CR1_OFFSET);
    volatile uint32_t *uart_brr = (volatile uint32_t *)(UART_BASE + UART_BRR_OFFSET);

    // Clear any previous errors
    uart_error_status = UART_ERROR_NONE;

    // Set baud rate
    *uart_brr = UART_BRR_VALUE(baud_rate);

    // Enable transmit and receive
    *uart_cr1 |= (UART_CR1_TE | UART_CR1_RE);

    // Initialize state to idle
    uart_state = UART_STATE_IDLE;

    return UART_ERROR_NONE;
}

UART_Error_t UART_TransmitByte(uint8_t data)
{
    volatile uint32_t *uart_sr = (volatile uint32_t *)(UART_BASE + UART_SR_OFFSET);
    volatile uint32_t *uart_dr = (volatile uint32_t *)(UART_BASE + UART_DR_OFFSET);

    // Wait until the transmit data register is empty
    while ((*uart_sr & UART_SR_TXE) == 0);

    // Transmit the data
    *uart_dr = data;

    // Update state to transmitting
    uart_state = UART_STATE_TRANSMITTING;

    return UART_ERROR_NONE;
}

UART_Error_t UART_ReceiveByte(uint8_t *ptr_data)
{
    volatile uint32_t *uart_sr = (volatile uint32_t *)(UART_BASE + UART_SR_OFFSET);
    volatile uint32_t *uart_dr = (volatile uint32_t *)(UART_BASE + UART_DR_OFFSET);

    // Wait until the receive data register is not empty
    while ((*uart_sr & UART_SR_RXNE) == 0);

    // Check for errors
    if (*uart_sr & UART_SR_ORE) {
        uart_error_status |= UART_ERROR_OVERRUN;
    }
    if (*uart_sr & UART_SR_FE) {
        uart_error_status |= UART_ERROR_FRAMING;
    }
    if (*uart_sr & UART_SR_NE) {
        uart_error_status |= UART_ERROR_NOISE;
    }

    // Receive the data
    *ptr_data = (uint8_t)(*uart_dr);

    // Update state to receiving
    uart_state = UART_STATE_RECEIVING;

    return uart_error_status;
}

UART_Error_t UART_GetErrorStatus(void)
{
    return uart_error_status;
}

void UART_ClearErrors(UART_Error_t error_mask)
{
    uart_error_status &= ~error_mask;
}