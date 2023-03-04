#include "SIM868.h"

int _write(int file, char *ptr, int len) {
  int DataIdx;
  for (DataIdx = 0; DataIdx < len; DataIdx++) {
    ITM_SendChar(*ptr++);
  }
  return len;
}

uint8_t rxBuffer[BUFFER_SIZE] = {0,};
static UART_HandleTypeDef *huart;
static void (*error_cb)();

static void SIM868_ERROR(const char *error_msg)
{
	printf(error_msg);
	error_cb();
}

void SIM868_init_lib(UART_HandleTypeDef *uart_handle, void (*hl_error_handler)())
{
	error_cb = hl_error_handler;
	if (uart_handle->Init.BaudRate != BAUD_RATE) {
		SIM868_ERROR("[SIM868_init_lib] uart_handle baud rate is incorrect\n");
	}
	huart = uart_handle;
}

/* LL functions ------------------------------------------------------------------*/
SIM_StatusTypeDef SIM868_transmit_to(const char* cmd, uint16_t timeout)
{
	printf("%s->\n", cmd);
	return HAL_UART_Transmit(huart, cmd, strlen(cmd), timeout);
}

SIM_StatusTypeDef SIM868_recieve_to(uint16_t timeout)
{
	if (HAL_UART_Receive(huart, rxBuffer, 1, timeout) != HAL_OK) {
		printf("NO RESPONSE\n");
		return SIM_ERROR;
	}
	uint8_t i = 1;
	for (; HAL_UART_Receive(huart, &rxBuffer[i], 1, UART_RECEIVE_TIMEOUT) == HAL_OK; ++i);
	rxBuffer[i] = '\0';

	printf("%s\n", rxBuffer);
	return SIM_OK;
}

/* ML functions ------------------------------------------------------------------*/
SIM_StatusTypeDef SIM868_cmd(const char* cmd, const char* reply)
{
	return SIM868_cmd_to(cmd, reply, 2500);
}

SIM_StatusTypeDef SIM868_cmd_to(const char* cmd, const char* reply, uint16_t timeout)
{
	SIM868_transmit_to(cmd, UART_TRANSMIT_TIMEOUT);
	return SIM868_recieve_reply(reply, timeout);
}

SIM_StatusTypeDef SIM868_recieve_reply(const char* reply, uint16_t timeout)
{
	if (SIM868_recieve_to(timeout) != SIM_OK) {
		return SIM_ERROR;
	}

	return (strstr(rxBuffer, reply) != NULL) ? SIM_OK : SIM_ERROR;
}

// TODO: retry on ERROR
SIM_StatusTypeDef SIM868_exec_cmd_chain(char *cmd_chain[], char *reply_chain[], uint16_t timeout_chain[],
										uint16_t wait_time_chain[], uint16_t cmd_num)
{
	for (uint16_t i = 0; i < cmd_num; ++i) {
		if (SIM868_cmd_to(cmd_chain[i], reply_chain[i], timeout_chain[i]) != SIM_OK) {
			return SIM_ERROR;
		}
		HAL_Delay(wait_time_chain[i]);
	}

	return SIM_OK;
}

/* HL functions ------------------------------------------------------------------*/
void SIM868_check_connection(void)
{
	SIM868_transmit_to("AT\r", UART_TRANSMIT_TIMEOUT);
	if (SIM868_recieve_to(1000) != SIM_OK) {
		SIM868_ERROR("[SIM868_check_connection] Module is not connected\n");
	}

	if (strstr(rxBuffer, "OK") != NULL) return;
	if (strstr(rxBuffer, "ERROR") != NULL) {
		if (SIM868_cmd_to("AT\r", "OK", 1000) != SIM_OK) {
			SIM868_ERROR("[SIM868_check_connection] Module connection is corrupted\n");
		}
	}
}

void SIM868_init(void)
{
#ifdef NO_SYNC_STARTUP
	SIM868_check_connection();
#else
	if (SIM868_recieve_reply("READY", 30000) != SIM_OK) {
		SIM868_check_connection();
	}
#endif

	uint8_t try_cnt = 0;
	while (SIM868_cmd("AT+CREG?\r", "CREG:") != SIM_OK &&
			rxBuffer[CREG_POS] != '1' && try_cnt < 5) {
		++try_cnt;
		HAL_Delay(2000);
	}
	if (try_cnt == 5) SIM868_ERROR("[SIM868_init] Too many tries (5) on AT+CREG: x,1\n");

	try_cnt = 0;
	while (SIM868_cmd("AT+CGREG?\r", "CGREG:") != SIM_OK &&
			rxBuffer[CGREG_POS] != '1' && try_cnt < 5) {
		++try_cnt;
		HAL_Delay(2000);
	}
	if (try_cnt == 5) SIM868_ERROR("[SIM868_init] Too many tries (5) on AT+CGREG: x,1\n");
}

void SIM868_GPRS_init(void)
{
	SIM868_init();

	char *cmd_chain[] = {
	"AT+CREG=1\r", "AT+CREG?\r", "AT+CGREG=1\r", "AT+CGREG?\r", "AT+CGATT=1\r", "AT+CGATT?\r",
	"AT+CSTT=\"internet\",\"\",\"\"\r", "AT+CIICR\r", "AT+CIFSR\r"
	};
	char *reply_chain[] = {
	"OK", "CREG: 1,1", "OK", "CGREG: 1,1", "OK", "CGATT: 1", "OK", "OK", "."
	};
	uint16_t timeout_chain[] = {
	2500, 2500, 2500, 2500, 2500, 2500, 10000, 5000, 5000
	};
	uint16_t wait_time_chain[] = {
	1000, 0, 1000, 0, 5000, 0, 5000, 2500, 0
	};

	if (SIM868_exec_cmd_chain(cmd_chain, reply_chain, timeout_chain, wait_time_chain, 8) != SIM_OK) {
		SIM868_ERROR("[SIM868_GPRS_init] exec_cmd_chain ERROR");
	}
}
