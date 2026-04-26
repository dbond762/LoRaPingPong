#ifndef RADIO_CONFIG_H
#define RADIO_CONFIG_H

#include "sx128x.h"
#include "sx128x_hal.h"
#include <stdint.h>
#include "lora_protocol.h"

/*
 *  Параметри радіо:
 *    Частота : 2450 МГц
 *    SF      : 6  (SF6)
 *    BW      : 800 кГц
 *    CR      : 4/6
 *    CRC     : увімкнено (апаратний)
 *    Потужність: +13 дБм (макс для SX1280)
 */

#define RADIO_FREQ_HZ       2450000000UL
#define RADIO_TX_POWER_DBM  13
#define RADIO_RAMP_TIME     SX128X_RAMP_20_US

/* Timeout RX: 0xFFFF = continuous */
#define RADIO_RX_CONTINUOUS  0xFFFF
/* Timeout TX: 0 = single shot (до кінця пакету) */
#define RADIO_TX_SINGLE      0

/**
 * @brief Ініціалізувати SX1280: standby → LoRa → freq → mod → pkt → power → IRQ
 * @param context  Вказівник контексту (передається в кожну функцію SWDR005)
 *                 Якщо не використовуєш context — передай NULL
 * @return SX128X_STATUS_OK або код помилки
 */
sx128x_status_t Radio_Init(const void *context);

/**
 * @brief Встановити IRQ маски (TxDone + RxDone + CrcError на DIO1)
 */
sx128x_status_t Radio_SetIrq(const void *context);

#endif /* RADIO_CONFIG_H */
