#include <stdint.h>
#include <stdio.h>

// Mocked registers
static uint32_t mock_regs[16];
#define UART_BASE ((uintptr_t)mock_regs)

// Define SystemCoreClock as required by the module
uint32_t SystemCoreClock = 16000000u;

// Include the source file directly to use the fake registers
#include "uart_comm.c"

#include "unity.h"

static void setUp(void) {
    // Pre-set the status register bits
    mock_regs[(UART_LSR - UART_BASE) >> 2] = UART_LSR_THRE | UART_LSR_DR;
    last_error = UART_NO_ERROR;
}

static void tearDown(void) {
    // Nothing to tear down
}

TEST_GROUP(UART);

TEST(UART, UART_Init_ConfigError) {
    UART_Config_t config = { .baud_rate = 9600, .parity = 0, .stop_bits = 1, .hardware_flow_control = 0 };
    TEST_ASSERT_EQUAL(UART_CONFIG_ERROR, UART_Init(&config));
    TEST_ASSERT_EQUAL(UART_CONFIG_ERROR, last_error);
}

TEST(UART, UART_Init_NoError) {
    UART_Config_t config = { .baud_rate = 115200, .parity = 0, .stop_bits = 1, .hardware_flow_control = 0 };
    TEST_ASSERT_EQUAL(UART_NO_ERROR, UART_Init(&config));
    TEST_ASSERT_EQUAL(UART_NO_ERROR, last_error);
}

TEST(UART, UART_Transmit_SingleByte) {
    uint8_t data = 0x7F;
    UART_Transmit(&data, 1);
    TEST_ASSERT_EQUAL(0x7F, mock_regs[(UART_THR - UART_BASE) >> 2]);
}

TEST(UART, UART_Transmit_MultipleBytes) {
    uint8_t data[] = { 0x01, 0x02, 0x03 };
    UART_Transmit(data, 3);
    TEST_ASSERT_EQUAL(0x03, mock_regs[(UART_THR - UART_BASE) >> 2]);  // Last byte should be in the THR register
}

TEST(UART, UART_Receive_NoData) {
    uint8_t buffer[3];
    uint16_t received_length;
    mock_regs[(UART_LSR - UART_BASE) >> 2] = 0;  // No data ready
    UART_Receive(buffer, sizeof(buffer), &received_length);
    TEST_ASSERT_EQUAL(0, received_length);
}

TEST(UART, UART_Receive_SingleByte) {
    uint8_t buffer[3];
    uint16_t received_length;
    mock_regs[(UART_RBR - UART_BASE) >> 2] = 0x7F;
    mock_regs[(UART_LSR - UART_BASE) >> 2] = UART_LSR_DR;  // Data ready
    UART_Receive(buffer, sizeof(buffer), &received_length);
    TEST_ASSERT_EQUAL(1, received_length);
    TEST_ASSERT_EQUAL(0x7F, buffer[0]);
}

TEST(UART, UART_Receive_MultipleBytes) {
    uint8_t buffer[3];
    uint16_t received_length;
    mock_regs[(UART_RBR - UART_BASE) >> 2] = 0x01;
    mock_regs[(UART_LSR - UART_BASE) >> 2] = UART_LSR_DR;  // Data ready
    UART_Receive(buffer, sizeof(buffer), &received_length);
    TEST_ASSERT_EQUAL(1, received_length);
    TEST_ASSERT_EQUAL(0x01, buffer[0]);

    mock_regs[(UART_RBR - UART_BASE) >> 2] = 0x02;
    mock_regs[(UART_LSR - UART_BASE) >> 2] = UART_LSR_DR;  // Data ready
    UART_Receive(buffer, sizeof(buffer), &received_length);
    TEST_ASSERT_EQUAL(2, received_length);
    TEST_ASSERT_EQUAL(0x02, buffer[1]);

    mock_regs[(UART_RBR - UART_BASE) >> 2] = 0x03;
    mock_regs[(UART_LSR - UART_BASE) >> 2] = UART_LSR_DR;  // Data ready
    UART_Receive(buffer, sizeof(buffer), &received_length);
    TEST_ASSERT_EQUAL(3, received_length);
    TEST_ASSERT_EQUAL(0x03, buffer[2]);
}

TEST(UART, UART_GetError_NoError) {
    TEST_ASSERT_EQUAL(UART_NO_ERROR, UART_GetError());
}

TEST(UART, UART_GetError_ConfigError) {
    UART_Config_t config = { .baud_rate = 9600, .parity = 0, .stop_bits = 1, .hardware_flow_control = 0 };
    UART_Init(&config);  // Set config error
    TEST_ASSERT_EQUAL(UART_CONFIG_ERROR, UART_GetError());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(UART_Init_ConfigError);
    RUN_TEST(UART_Init_NoError);
    RUN_TEST(UART_Transmit_SingleByte);
    RUN_TEST(UART_Transmit_MultipleBytes);
    RUN_TEST(UART_Receive_NoData);
    RUN_TEST(UART_Receive_SingleByte);
    RUN_TEST(UART_Receive_MultipleBytes);
    RUN_TEST(UART_GetError_NoError);
    RUN_TEST(UART_GetError_ConfigError);
    return UNITY_END();
}
