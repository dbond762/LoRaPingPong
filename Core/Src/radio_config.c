#include "radio_config.h"

sx128x_status_t Radio_Init(const void *context)
{
	sx128x_status_t st;

	sx128x_hal_reset(context);

	st = sx128x_set_standby(context, SX128X_STANDBY_CFG_RC);
	if (st != SX128X_STATUS_OK)
	{
		return st;
	}

	st = sx128x_set_pkt_type(context, SX128X_PKT_TYPE_LORA);
	if (st != SX128X_STATUS_OK)
	{
		return st;
	}

	st = sx128x_set_rf_freq(context, RADIO_FREQ_HZ);
	if (st != SX128X_STATUS_OK)
	{
		return st;
	}

	st = sx128x_set_buffer_base_address(context, 0x00, 0x80);
	if (st != SX128X_STATUS_OK)
	{
		return st;
	}

	sx128x_mod_params_lora_t mod =
	{
		.sf = SX128X_LORA_RANGING_SF6,
		.bw = SX128X_LORA_RANGING_BW_800,
		.cr = SX128X_LORA_RANGING_CR_4_6,
	};
	st = sx128x_set_lora_mod_params(context, &mod);
	if (st != SX128X_STATUS_OK)
	{
		return st;
	}

	sx128x_pkt_params_lora_t pkt =
	{
		.preamble_len     = { .mant = 6, .exp = 1 },
		.header_type      = SX128X_LORA_RANGING_PKT_EXPLICIT,
		.pld_len_in_bytes = LP_MAX_PACKET,
		.crc_is_on        = true,
		.invert_iq_is_on  = false,
	};
	st = sx128x_set_lora_pkt_params(context, &pkt);
	if (st != SX128X_STATUS_OK)
	{
		return st;
	}

	st = sx128x_set_tx_params(context, RADIO_TX_POWER_DBM, RADIO_RAMP_TIME);
	if (st != SX128X_STATUS_OK)
	{
		return st;
	}

	st = Radio_SetIrq(context);

	return st;
}

sx128x_status_t Radio_SetIrq(const void *context)
{
	uint16_t irq_mask = SX128X_IRQ_TX_DONE | SX128X_IRQ_RX_DONE
			| SX128X_IRQ_CRC_ERROR | SX128X_IRQ_HEADER_ERROR
			| SX128X_IRQ_TIMEOUT;

	return sx128x_set_dio_irq_params(
		context, irq_mask, irq_mask, 0x0000, 0x0000
	);
}
