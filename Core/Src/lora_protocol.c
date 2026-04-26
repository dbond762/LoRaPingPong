#include "lora_protocol.h"
#include <string.h>

/* CRC16-CCITT Kermit (poly=0x1021, init=0x0000, RefIn, RefOut) */
uint16_t LP_CRC16(const uint8_t *data, uint16_t len)
{
	uint16_t crc = 0x0000;
	for (uint16_t i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (uint8_t b = 0; b < 8; b++)
		{
			crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : (crc >> 1);
		}
	}
	return crc;
}

uint8_t LP_Build(uint8_t *buf, LP_Type_t type, uint8_t seq, uint8_t src,
		uint8_t dst, const uint8_t *payload, uint8_t plen)
{
	if (plen < LP_MIN_PAYLOAD)
	{
		plen = LP_MIN_PAYLOAD;
	}
	if (plen > LP_MAX_PAYLOAD)
	{
		return 0;
	}

	buf[0] = LP_MAGIC;
	buf[1] = LP_VERSION;
	buf[2] = (uint8_t) type;
	buf[3] = seq;
	buf[4] = src;
	buf[5] = dst;
	buf[6] = plen;

	if (payload != NULL && plen > 0)
	{
		memcpy(&buf[LP_HEADER_SIZE], payload, plen);
	}

	uint8_t n = LP_HEADER_SIZE + plen;
	uint16_t crc = LP_CRC16(buf, n);
	buf[n] = (uint8_t) (crc & 0xFF);
	buf[n + 1] = (uint8_t) (crc >> 8);

	return n + LP_CRC_SIZE;
}

bool LP_Parse(const uint8_t *raw, uint8_t len, LP_Packet_t *pkt)
{
	if (len < LP_MIN_PACKET)
	{
		return false;
	}
	if (raw[0] != LP_MAGIC)
	{
		return false;
	}
	if (raw[1] != LP_VERSION)
	{
		return false;
	}

	uint8_t plen = raw[6];
	if (plen > LP_MAX_PAYLOAD)
	{
		return false;
	}

	uint8_t expected = LP_HEADER_SIZE + plen + LP_CRC_SIZE;
	if (len < expected)
	{
		return false;
	}

	uint16_t crc_rx = (uint16_t) raw[LP_HEADER_SIZE + plen]
			| ((uint16_t) raw[LP_HEADER_SIZE + plen + 1] << 8);
	uint16_t crc_calc = LP_CRC16(raw, LP_HEADER_SIZE + plen);

	pkt->crc_ok = (crc_rx == crc_calc);
	pkt->type = raw[2];
	pkt->seq  = raw[3];
	pkt->src  = raw[4];
	pkt->dst  = raw[5];
	pkt->payload_len = plen;
	if (plen > 0)
	{
		memcpy(pkt->payload, &raw[LP_HEADER_SIZE], plen);
	}

	return pkt->crc_ok;
}
