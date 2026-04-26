#include "sx128x_hal.h"
#include "stm32f4xx_hal.h"

extern SPI_HandleTypeDef hspi1;

#define NSS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define NSS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define RESET_LOW()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)
#define RESET_HIGH() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)
#define BUSY_READ() HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1)

#define BUSY_TIMEOUT_MS 100

static sx128x_hal_status_t wait_busy(void)
{
	uint32_t t0 = HAL_GetTick();
	while (BUSY_READ() == GPIO_PIN_SET)
	{
		if ((HAL_GetTick() - t0) > BUSY_TIMEOUT_MS)
		{
			return SX128X_HAL_STATUS_ERROR;
		}
	}
	return SX128X_HAL_STATUS_OK;
}

sx128x_hal_status_t sx128x_hal_write(const void *context,
		const uint8_t *command, const uint16_t command_length,
		const uint8_t *data, const uint16_t data_length)
{
	sx128x_hal_status_t st = wait_busy();
	if (st != SX128X_HAL_STATUS_OK)
	{
		return st;
	}

	NSS_LOW();
	HAL_SPI_Transmit(&hspi1, (uint8_t*) command, command_length, HAL_MAX_DELAY);
	if (data_length > 0 && data != NULL)
	{
		HAL_SPI_Transmit(&hspi1, (uint8_t*) data, data_length, HAL_MAX_DELAY);
	}
	NSS_HIGH();

	return SX128X_HAL_STATUS_OK;
}

sx128x_hal_status_t sx128x_hal_read(const void *context, const uint8_t *command,
		const uint16_t command_length, uint8_t *data,
		const uint16_t data_length)
{
	sx128x_hal_status_t st = wait_busy();
	if (st != SX128X_HAL_STATUS_OK)
	{
		return st;
	}

	NSS_LOW();
	HAL_SPI_Transmit(&hspi1, (uint8_t*) command, command_length, HAL_MAX_DELAY);
	HAL_SPI_Receive(&hspi1, data, data_length, HAL_MAX_DELAY);
	NSS_HIGH();

	return SX128X_HAL_STATUS_OK;
}

sx128x_hal_status_t sx128x_hal_reset(const void *context)
{
	RESET_LOW();
	HAL_Delay(10);
	RESET_HIGH();
	HAL_Delay(20);
	return wait_busy();
}

sx128x_hal_status_t sx128x_hal_wakeup(const void *context)
{
	NSS_LOW();
	HAL_Delay(1);
	NSS_HIGH();
	return wait_busy();
}
