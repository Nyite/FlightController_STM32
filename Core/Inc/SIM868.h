#ifndef __SIM868__
#define __SIM868__

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define NO_SYNC_STARTUP
#define BUFFER_SIZE 64

#define BAUD_RATE 9600
#define UART_TRANSMIT_TIMEOUT 100
#define UART_RECEIVE_TIMEOUT 10
#define CREG_POS 10
#define CGREG_POS 11

typedef enum
{
  SIM_OK       = 0x00U,
  SIM_ERROR    = 0x01U
} SIM_StatusTypeDef;

void SIM868_init_lib(UART_HandleTypeDef *uart_handle, void (*hl_error_handler)());

/* LL functions ------------------------------------------------------------------*/
/** @brief Low level functions that make basic operations with module and library
 *
 *  @retval SIM_StatusTypeDef
  */
SIM_StatusTypeDef SIM868_transmit_to(const char* cmd, uint16_t timeout);
SIM_StatusTypeDef SIM868_recieve_to(uint16_t timeout);

/* ML functions ------------------------------------------------------------------*/
/** @brief Middle level functions that make combinations of LL operations with module
 *
 *  @retval SIM_StatusTypeDef
  */
SIM_StatusTypeDef SIM868_cmd(const char* cmd, const char* reply);
SIM_StatusTypeDef SIM868_cmd_to(const char* cmd, const char* reply, uint16_t timeout);
SIM_StatusTypeDef SIM868_recieve_reply(const char* reply, uint16_t timeout);
SIM_StatusTypeDef SIM868_exec_cmd_chain(char *cmd_chain[], char *reply_chain[], uint16_t timeout_chain[],
										uint16_t wait_time_chain[], uint16_t cmd_num);

/* HL functions ------------------------------------------------------------------*/
/** @brief Middle level functions that make combinations of ML operations with module
 *
 *  @note Execute error_cb on error
 *  @retval None
  */
void SIM868_check_connection(void);
void SIM868_init(void);
void SIM868_GPRS_init(void);

#endif // __SIM868__
