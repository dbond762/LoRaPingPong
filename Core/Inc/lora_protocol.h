#ifndef LORA_PROTOCOL_H
#define LORA_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ═══════════════════════════════════════════════════════════════
 *  Packet:
 *
 *  [0]   MAGIC   = 0xA5
 *  [1]   VERSION = 0x01
 *  [2]   TYPE    = DATA/ACK
 *  [3]   SEQ     = sequence number (0..255, wrap)
 *  [4]   SRC     = Source ID
 *  [5]   DST     = Destination ID
 *  [6]   PLEN    = payload length
 *  [7..] PAYLOAD
 *  [last-1, last] CRC16-CCITT (Kermit)
 * ═══════════════════════════════════════════════════════════════
 */

#define LP_MAGIC        0xA5
#define LP_VERSION      0x01

#define LP_HEADER_SIZE  7
#define LP_CRC_SIZE     2
#define LP_MIN_PAYLOAD  3
#define LP_MAX_PAYLOAD  100
#define LP_MAX_PACKET   (LP_HEADER_SIZE + LP_MAX_PAYLOAD + LP_CRC_SIZE)
#define LP_MIN_PACKET   (LP_HEADER_SIZE + LP_MIN_PAYLOAD + LP_CRC_SIZE) /* = 12 */

typedef enum
{
	LP_TYPE_DATA = 0x01,
	LP_TYPE_ACK  = 0x02,
} LP_Type_t;

#define LP_NODE_A  0x01
#define LP_NODE_B  0x02

typedef struct
{
	uint8_t type;
	uint8_t seq;
	uint8_t src;
	uint8_t dst;
	uint8_t payload[LP_MAX_PAYLOAD];
	uint8_t payload_len;
	bool crc_ok;
} LP_Packet_t;

/* API */
uint16_t LP_CRC16(const uint8_t *data, uint16_t len);

uint8_t LP_Build(uint8_t *buf, LP_Type_t type, uint8_t seq, uint8_t src,
		uint8_t dst, const uint8_t *payload, uint8_t plen);

bool LP_Parse(const uint8_t *raw, uint8_t len, LP_Packet_t *pkt);

#endif /* LORA_PROTOCOL_H */
