// UART_Driver_test.c
#include "unity.h"
#include "UART_Driver.h"
#include "CMock.h"

// Mocking the volatile registers
#define SystemCoreClock 8000000 // 8 MHz System Clock for simulation purposes
#define UART_BASE 0x4000C000
#define UART_CR1_OFFSET 0x00000000
#define UART_DR_OFFSET  0x00000004
#define UART_SR_OFFSET  0x0000000C
#define UART_BRR_OFFSET 0x00000008

volatile uint32_t uart_cr1 = 0;
volatile uint32_t uart_dr = 0;
volatile uint32_t uart_sr = 0;
volatile uint32_t uart_brr = 0;

volatile uint32_t *const uart_cr1_ptr = (volatile uint32_t *)(UART_BASE + UART_CR1_OFFSET);
volatile uint32_t *const uart_dr_ptr = (volatile uint32_t *)(UART_BASE + UART_DR_OFFSET);
volatile uint32_t *const uart_sr_ptr = (volatile uint32_t *)(UART_BASE + UART_SR_OFFSET);
volatile uint32_t *const uart_brr_ptr = (volatile uint32_t *)(UART_BASE + UART_BRR_OFFSET);

TEST_GROUP(UART_Driver);

TEST_SETUP(UART_Driver)
{
    uart_cr1 = 0;
    uart_dr = 0;
    uart_sr = 0;
    uart_brr = 0;
    uart_error_status = UART_ERROR_NONE;
    uart_state = UART_STATE_IDLE;
}

TEST_TEAR_DOWN(UART_Driver)
{
    // Nothing to clean up in this setup
}

TEST(UART_Driver, UART_Init_InitializesBaudRateAndEnablesTransmitReceive)
{
    // Expected Baud Rate Setting
    uint32_t expected_brr = UART_BRR_VALUE(UART_BAUD_RATE);
    
    UART_Init(UART_BAUD_RATE);

    TEST_ASSERT_EQUAL_UINT32(expected_brr, uart_brr);
    TEST_ASSERT_EQUAL_UINT32((UART_CR1_TE | UART_CR1_RE), uart_cr1);
    TEST_ASSERT_EQUAL_UINT32(UART_STATE_IDLE, uart_state);
}

TEST(UART_Driver, UART_Init_InitializesWithErrorStatusNone)
{
    UART_Init(UART_BAUD_RATE);
    TEST_ASSERT_EQUAL_UINT32(UART_ERROR_NONE, uart_error_status);
}

TEST(UART_Driver, UART_TransmitByte_TransmitsDataAndUpdatesState)
{
    uint8_t data_to_send = 0x55;

    UART_TransmitByte(data_to_send);

    TEST_ASSERT_EQUAL_UINT8(data_to_send, uart_dr);
    TEST_ASSERT_EQUAL_UINT32(UART_STATE_TRANSMITTING, uart_state);
}

TEST(UART_Driver, UART_TransmitByte_WaitsForTxeBit)
{
    uint8_t data_to_send = 0xAA;
    uart_sr &= ~UART_SR_TXE; // TXE bit not set initially

    UART_TransmitByte(data_to_send);

    TEST_ASSERT_EQUAL_UINT8(data_to_send, uart_dr);
    TEST_ASSERT_EQUAL_UINT32(UART_STATE_TRANSMITTING, uart_state);
}

TEST(UART_Driver, UART_ReceiveByte_ReceivesDataAndChecksForErrors)
{
    uint8_t received_data = 0x77;
    uart_dr = received_data;
    uart_sr |= UART_SR_RXNE; // RXNE bit set
    uart_sr |= UART_SR_ORE;  // Overrun error
    uart_sr |= UART_SR_FE;   // Framing error
    uart_sr |= UART_SR_NE;   // Noise error

    uint8_t ptr_data;
    UART_Error_t result = UART_ReceiveByte(&ptr_data);

    TEST_ASSERT_EQUAL_UINT8(received_data, ptr_data);
    TEST_ASSERT_EQUAL_UINT32((UART_ERROR_OVERRUN | UART_ERROR_FRAMING | UART_ERROR_NOISE), result);
    TEST_ASSERT_EQUAL_UINT32(UART_STATE_RECEIVING, uart_state);
}

TEST(UART_Driver, UART_ReceiveByte_WaitsForRxneBit)
{
    uint8_t received_data = 0xFE;
    uart_dr = received_data;
    uart_sr &= ~UART_SR_RXNE; // RXNE bit not set initially

    uint8_t ptr_data;
    UART_ReceiveByte(&ptr_data);

    TEST_ASSERT_EQUAL_UINT8(received_data, ptr_data);
    TEST_ASSERT_EQUAL_UINT32(UART_STATE_RECEIVING, uart_state);
}

TEST(UART_Driver, UART_GetErrorStatus_ReturnsCurrentErrorStatus)
{
    uart_error_status |= UART_ERROR_OVERRUN;
    uart_error_status |= UART_ERROR_FRAMING;

    UART_Error_t error_status = UART_GetErrorStatus();

    TEST_ASSERT_EQUAL_UINT32((UART_ERROR_OVERRUN | UART_ERROR_FRAMING), error_status);
}

TEST(UART_Driver, UART_ClearErrors_ClearsSpecifiedErrors)
{
    uart_error_status |= UART_ERROR_OVERRUN;
    uart_error_status |= UART_ERROR_FRAMING;

    UART_ClearErrors(UART_ERROR_OVERRUN);

    TEST_ASSERT_EQUAL_UINT32(UART_ERROR_FRAMING, uart_error_status);
}