#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "unity.h"
#include "uart_comm.h"

// Mock UART registers
volatile uint32_t *UART_THR = (volatile uint32_t *)0x4000C000; // Transmitter Holding Register
volatile uint32_t *UART_RBR = (volatile uint32_t *)0x4000C000; // Receiver Buffer Register
volatile uint32_t *UART_LCR = (volatile uint32_t *)0x4000C00C; // Line Control Register
volatile uint32_t *UART_LSR = (volatile uint32_t *)0x4000C014; // Line Status Register
volatile uint32_t *UART_UBR = (volatile uint32_t *)0x4000C02C; // UART Baud Rate Register

uint32_t mock_UART_THR_value = 0;
uint32_t mock_UART_RBR_value = 0;
uint32_t mock_UART_LSR_value = 0;
uint32_t mock_UART_UBR_value = 0;

void setUp(void) {
    *UART_THR = 0;
    *UART_RBR = 0;
    *UART_LCR = 0;
    *UART_LSR = 0;
    *UART_UBR = 0;

    mock_UART_THR_value = 0;
    mock_UART_RBR_value = 0;
    mock_UART_LSR_value = 0;
    mock_UART_UBR_value = 0;
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

    TEST_ASSERT_EQUAL_HEX32(*UART_LCR, 0x03); // 8 data bits, no parity, 1 stop bit
    TEST_ASSERT_EQUAL_HEX32(*UART_UBR, 0x1C); // Baud rate divisor for 115200 at 16 MHz clock
}

void test_UART_Transmit(void) {
    *UART_LSR = UART_LSR_THRE; // Transmit Holding Register Empty

    UART_Transmit(0x55);

    TEST_ASSERT_EQUAL_UINT32(*UART_THR, 0x55);
}

void test_UART_Transmit_TransmitNotReady(void) {
    *UART_LSR = 0; // No flags set

    UART_Status status = UART_Transmit(0x55);

    TEST_ASSERT_EQUAL(status, UART_STATUS_ERROR_TIMEOUT);
}

void test_UART_Receive(void) {
    *UART_LSR = UART_LSR_DR; // Data Ready
    *UART_RBR = 0xAA;

    uint8_t received;
    UART_Receive(&received);

    TEST_ASSERT_EQUAL_UINT8(received, 0xAA);
}

void test_UART_Receive_NoDataReady(void) {
    *UART_LSR = 0; // No flags set

    uint8_t received;
    UART_Receive(&received);

    TEST_ASSERT_EQUAL_UINT8(received, 0x00); // Undefined behavior, but we check for no data
}

void test_UART_TransmitString(void) {
    char *test_string = "Hello\n";
    UART_TransmitString(test_string);

    TEST_ASSERT_EQUAL_UINT32(*UART_THR, 'H');
    TEST_ASSERT_EQUAL_UINT32(*(UART_THR + 1), 'e');
    TEST_ASSERT_EQUAL_UINT32(*(UART_THR + 2), 'l');
    TEST_ASSERT_EQUAL_UINT32(*(UART_THR + 3), 'l');
    TEST_ASSERT_EQUAL_UINT32(*(UART_THR + 4), 'o');
    TEST_ASSERT_EQUAL_UINT32(*(UART_THR + 5), '\n');
}

void test_UART_ReceiveString(void) {
    *UART_LSR = UART_LSR_DR; // Data Ready
    *UART_RBR = 'H';
    UART_Receive(&mock_UART_RBR_value);
    *UART_RBR = 'e';
    UART_Receive(&mock_UART_RBR_value);
    *UART_RBR = 'l';
    UART_Receive(&mock_UART_RBR_value);
    *UART_RBR = 'l';
    UART_Receive(&mock_UART_RBR_value);
    *UART_RBR = 'o';
    UART_Receive(&mock_UART_RBR_value);
    *UART_RBR = '\n';
    UART_Receive(&mock_UART_RBR_value);

    char buffer[10];
    UART_ReceiveString(buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_STRING(buffer, "Hello\n");
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
