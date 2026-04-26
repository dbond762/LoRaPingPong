#include "link.h"

static const void *radio_ctx = NULL;

static uint8_t tx_buf[LP_MAX_PACKET];
static uint8_t rx_buf[LP_MAX_PACKET];

static uint8_t  tx_seq   = 0;
static uint32_t tx_count = 0;
static uint32_t ok_count = 0;

static uint8_t payload[LP_MAX_PAYLOAD];

static uint8_t make_payload(uint8_t *buf)
{
	uint32_t t = HAL_GetTick();

	buf[0] = tx_seq;
	buf[1] = (uint8_t) (t >> 8);
	buf[2] = (uint8_t) (t);

	memcpy(&buf[3], "NodeA", 5);

	return 8;
}

static sx128x_irq_mask_t wait_irq(uint32_t timeout_ms, sx128x_irq_mask_t wanted)
{
	uint32_t t0 = HAL_GetTick();
	while ((HAL_GetTick() - t0) < timeout_ms)
	{
		sx128x_irq_mask_t irq = 0;
		sx128x_get_irq_status(radio_ctx, &irq);
		if (irq & wanted)
		{
			sx128x_clear_irq_status(radio_ctx, irq);
			return irq;
		}
	}
	sx128x_clear_irq_status(radio_ctx, SX128X_IRQ_ALL);
	return SX128X_IRQ_TIMEOUT;
}

static void radio_send(const uint8_t *buf, uint8_t len)
{
	sx128x_pkt_params_lora_t pkt_params =
	{
		.preamble_len     = { .mant = 6, .exp = 1 },
		.header_type      = SX128X_LORA_RANGING_PKT_EXPLICIT,
		.pld_len_in_bytes = len,
		.crc_is_on        = true,
		.invert_iq_is_on  = false,
	};
	sx128x_set_lora_pkt_params(radio_ctx, &pkt_params);
	sx128x_write_buffer(radio_ctx, 0x00, (uint8_t*) buf, len);
	sx128x_set_tx(radio_ctx, SX128X_TICK_SIZE_1000_US, RADIO_TX_SINGLE);

	uint32_t t0 = HAL_GetTick();
	while ((HAL_GetTick() - t0) < 500)
	{
		sx128x_irq_mask_t irq = 0;
		sx128x_get_irq_status(radio_ctx, &irq);
		if (irq & SX128X_IRQ_TX_DONE)
		{
			sx128x_clear_irq_status(radio_ctx, SX128X_IRQ_TX_DONE);
			break;
		}
	}
}

static void send_ack(const LP_Packet_t *req, int8_t rssi, int8_t snr)
{
	uint8_t pl[3] =
	{
		req->seq,
		(uint8_t) rssi,
		(uint8_t) snr
	};

	uint8_t len = LP_Build(
		tx_buf, LP_TYPE_ACK, req->seq, LP_NODE_B, LP_NODE_A, pl, sizeof(pl)
	);

	HAL_Delay(5);
	radio_send(tx_buf, len);
}

void node_a_init()
{
	printf("\r\n=== LoRa Node A | SF6 BW800 2450MHz ===\r\n");

	sx128x_status_t st = Radio_Init(radio_ctx);
	if (st != SX128X_STATUS_OK)
	{
		printf("[ERR] Radio init failed!\r\n");
		Error_Handler();
	}
	printf("[OK]  Radio ready\r\n");
}

void node_b_init()
{
	printf("\r\n=== LoRa Node B | SF6 BW800 2450MHz ===\r\n");

	sx128x_status_t st = Radio_Init(radio_ctx);
	if (st != SX128X_STATUS_OK)
	{
		printf("[ERR] Radio init failed!\r\n");
		Error_Handler();
	}
	printf("[OK]  Radio ready — listening...\r\n");

	sx128x_set_rx(radio_ctx, SX128X_TICK_SIZE_1000_US, RADIO_RX_CONTINUOUS);
}

void node_a_loop()
{
	uint8_t plen = make_payload(payload);
	uint8_t rawlen = LP_Build(tx_buf, LP_TYPE_DATA, tx_seq,
	LP_NODE_A, LP_NODE_B, payload, plen);

	printf("[TX]  seq=%3u len=%2u ... ", tx_seq, rawlen);

	sx128x_write_buffer(radio_ctx, 0x00, tx_buf, rawlen);

	sx128x_pkt_params_lora_t pkt_params =
	{
		.preamble_len     = { .mant = 6, .exp = 1 },
		.header_type      = SX128X_LORA_RANGING_PKT_EXPLICIT,
		.pld_len_in_bytes = rawlen,
		.crc_is_on        = true,
		.invert_iq_is_on  = false,
	};
	sx128x_set_lora_pkt_params(radio_ctx, &pkt_params);

	sx128x_set_tx(radio_ctx, SX128X_TICK_SIZE_1000_US, RADIO_TX_SINGLE);
	tx_count++;

	sx128x_irq_mask_t irq = wait_irq(500, SX128X_IRQ_TX_DONE);
	if (!(irq & SX128X_IRQ_TX_DONE))
	{
		printf("TX_TIMEOUT\r\n");
		tx_seq++;
		HAL_Delay(2000);
		return;
	}

	sx128x_set_rx(radio_ctx, SX128X_TICK_SIZE_1000_US, 1000);

	irq = wait_irq(1100,
			SX128X_IRQ_RX_DONE | SX128X_IRQ_CRC_ERROR | SX128X_IRQ_TIMEOUT);

	if (irq & SX128X_IRQ_CRC_ERROR)
	{
		printf("CRC_ERR\r\n");
		tx_seq++;
		HAL_Delay(2000);
		return;
	}
	if (!(irq & SX128X_IRQ_RX_DONE))
	{
		printf("ACK_TIMEOUT\r\n");
		tx_seq++;
		HAL_Delay(2000);
		return;
	}

	{
		sx128x_rx_buffer_status_t buf_st;
		sx128x_get_rx_buffer_status(radio_ctx, &buf_st);

		uint8_t rx_len = buf_st.pld_len_in_bytes;
		sx128x_read_buffer(radio_ctx, buf_st.buffer_start_pointer, rx_buf,
				rx_len);

		LP_Packet_t pkt;
		if (LP_Parse(rx_buf, rx_len, &pkt) && pkt.type == LP_TYPE_ACK
				&& pkt.src == LP_NODE_B && pkt.dst == LP_NODE_A
				&& pkt.seq == tx_seq)
		{
			ok_count++;
			sx128x_pkt_status_lora_t st_lora;
			sx128x_get_lora_pkt_status(radio_ctx, &st_lora);
			printf("ACK  RSSI=%d SNR=%d SR=%lu%%\r\n", st_lora.rssi,
					st_lora.snr, (ok_count * 100) / tx_count);
		}
		else
		{
			printf("BAD_ACK\r\n");
		}
	}

	tx_seq++;
	HAL_Delay(2000);
}

void node_b_loop()
{
	sx128x_irq_mask_t irq = 0;
	sx128x_get_irq_status(radio_ctx, &irq);

	if (!irq)
	{
		HAL_Delay(1);
		return;
	}

	if (irq & SX128X_IRQ_CRC_ERROR)
	{
		sx128x_clear_irq_status(radio_ctx, SX128X_IRQ_ALL);
		printf("[ERR] CRC error\r\n");
		sx128x_set_rx(radio_ctx, SX128X_TICK_SIZE_1000_US, RADIO_RX_CONTINUOUS);
		return;
	}

	if (irq & SX128X_IRQ_RX_DONE)
	{
		sx128x_clear_irq_status(radio_ctx, SX128X_IRQ_RX_DONE);

		sx128x_pkt_status_lora_t pkt_st;
		sx128x_get_lora_pkt_status(radio_ctx, &pkt_st);

		sx128x_rx_buffer_status_t buf_st;
		sx128x_get_rx_buffer_status(radio_ctx, &buf_st);

		uint8_t rx_len = buf_st.pld_len_in_bytes;
		sx128x_read_buffer(
			radio_ctx, buf_st.buffer_start_pointer, rx_buf, rx_len
		);

		LP_Packet_t pkt;
		if (!LP_Parse(rx_buf, rx_len, &pkt))
		{
			printf("[ERR] Bad packet len=%u\r\n", rx_len);
			sx128x_set_rx(
				radio_ctx, SX128X_TICK_SIZE_1000_US, RADIO_RX_CONTINUOUS
			);
			return;
		}

		if (pkt.dst != LP_NODE_B)
		{
			sx128x_set_rx(
				radio_ctx, SX128X_TICK_SIZE_1000_US, RADIO_RX_CONTINUOUS
			);
			return;
		}

		switch (pkt.type)
		{

		case LP_TYPE_DATA:
			printf(
				"[RX]  seq=%3u len=%2u RSSI=%d SNR=%d pld=",
				pkt.seq, rx_len, pkt_st.rssi, pkt_st.snr
			);
			for (uint8_t i = 0; i < pkt.payload_len; i++)
			{
				printf("%02X ", pkt.payload[i]);
			}
			printf("\r\n");

			send_ack(&pkt, pkt_st.rssi, pkt_st.snr);
			printf("[TX]  ACK seq=%u\r\n", pkt.seq);

			sx128x_set_rx(
				radio_ctx, SX128X_TICK_SIZE_1000_US, RADIO_RX_CONTINUOUS
			);
			break;

		default:
			sx128x_set_rx(
				radio_ctx, SX128X_TICK_SIZE_1000_US, RADIO_RX_CONTINUOUS
			);
			break;
		}
	}
	else
	{
		sx128x_clear_irq_status(radio_ctx, irq);
	}

	HAL_Delay(1);
}
