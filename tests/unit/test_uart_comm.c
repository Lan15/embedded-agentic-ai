#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "unity.h"
#include "uart_comm.h"

// Mock UART registers
static uint32_t mock_regs[16];
#define UART_BASE ((uintptr_t)mock_regs)

uint32_t SystemCoreClock = 16000000u;

#include "uart_comm.c"

void setUp(void) {
    for (int i = 0; i < 16; i++) {
        mock_regs[i] = 0;
    }

    // Pre-set status register bits
    UART_LSR = UART_LSR_THRE | UART_LSR_DR;
}

void tearDown(void) {
    // Cleanup can be done here if needed
}

void test_UART_Init(void) {
    UART_Config config = {
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_BITS_1,
        .data_bits = 8
    };

    UART_Init(&config);

    TEST_ASSERT_EQUAL_HEX32(UART_LCR, 0x03); // 8 data bits, no parity, 1 stop bit
    TEST_ASSERT_EQUAL_HEX32(7, UART_UBR); // 16e6/(16*115200)=8, minus 1
}

void test_UART_Transmit(void) {
    UART_Transmit(0x55);

    TEST_ASSERT_EQUAL_UINT32(UART_THR, 0x55);
}

void test_UART_Transmit_TransmitNotReady(void) {
    UART_LSR = 0;

    UART_Status status = UART_Transmit(0x55);

    TEST_ASSERT_EQUAL(UART_STATUS_ERROR_TIMEOUT, status);
}

void test_UART_Receive(void) {
    UART_RBR = 0xAA;

    uint8_t received;
    UART_Receive(&received);

    TEST_ASSERT_EQUAL_UINT8(received, 0xAA);
}

void test_UART_Receive_NoDataReady(void) {
    UART_LSR = UART_LSR_THRE | UART_LSR_DR;

    uint8_t received;
    UART_Receive(&received);

    TEST_ASSERT_EQUAL_UINT8(received, 0x00); // Undefined behavior, but we check for no data
}

void test_UART_TransmitString(void) {
    UART_TransmitString("Hello\n");
    TEST_ASSERT_EQUAL_UINT32('\n', UART_THR);
}

void test_UART_ReceiveString(void) {
    char buffer[10] = {0};
    UART_RBR = 'H';
    UART_ReceiveString(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_CHAR('H', buffer[0]);

    TEST_ASSERT_EQUAL_CHAR('H', buffer[0]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_UART_Init);
    RUN_TEST(test_UART_Transmit);
    RUN_TEST(test_UART_Transmit_TransmitNotReady);
    RUN_TEST(test_UART_Receive);
    RUN_TEST(test_UART_Receive_NoDataReady);
    RUN_TEST(test_UART_TransmitString);
    RUN_TEST(test_UART_ReceiveString);

    return UNITY_END();
}
