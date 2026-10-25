/* FILE: uart_comm.h */
#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>

/**
 * @brief Typedef for UART configuration structure
 */
typedef struct {
    uint32_t baud_rate;  /**< Baud rate */
    uint8_t parity;       /**< Parity setting */
    uint8_t stop_bits;    /**< Number of stop bits */
    uint8_t hardware_flow_control;  /**< Hardware flow control enable */
} UART_Config_t;

/**
 * @brief Enumeration for UART errors
 */
typedef enum {
    UART_NO_ERROR,       /**< No error */
    UART_CONFIG_ERROR,    /**< Configuration error */
    UART_TX_ERROR,        /**< Transmit error */
    UART_RX_ERROR         /**< Receive error */
} UART_Error_t;

/**
 * @brief Initialize the UART module
 *
 * This function initializes the UART module with the given configuration.
 *
 * @param config Configuration for the UART module
 * @return UART_Error_t Error code
 */
UART_Error_t UART_Init(const UART_Config_t *config);

/**
 * @brief Transmit data over UART
 *
 * This function transmits data over the UART interface.
 *
 * @param data Pointer to the data to be transmitted
 * @param length Number of bytes to transmit
 * @return UART_Error_t Error code
 * @req REQ-MCU-001 Transmit data over UART
 */
UART_Error_t UART_Transmit(const uint8_t *data, uint16_t length);

/**
 * @brief Receive data over UART
 *
 * This function receives data over the UART interface.
 *
 * @param buffer Pointer to the buffer to store received data
 * @param buffer_size Maximum number of bytes to receive
 * @param received_length Pointer to store the number of bytes actually received
 * @return UART_Error_t Error code
 * @req REQ-MCU-002 Receive data over UART
 */
UART_Error_t UART_Receive(uint8_t *buffer, uint16_t buffer_size, uint16_t *received_length);

/**
 * @brief Get last UART error
 *
 * This function retrieves the last error code from the UART module.
 *
 * @return UART_Error_t Error code
 * @req REQ-MCU-003 Get last UART error
 */
UART_Error_t UART_GetError(void);

#endif // UART_COMM_H
