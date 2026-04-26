# LoRaPingPong

Двонаправлений обмін пакетами (ping-pong) між двома вузлами на базі STM32F401CCU6 та радіомодуля SX1280 (2.4 GHz, модуляція LoRa SW6, BW800).

## Апаратура

| Компонент | Деталь |
|-----------|--------|
| MCU | STM32F401CCU6 |
| Радіо | SX1280 |
| Інтерфейс до радіо | SPI1 |
| Debug UART | USART2, 115200 8N1 |

### Пінаут SX1280 → STM32

| Сигнал | STM32 pin |
|--------|-----------|
| SPI SCK | PA5 |
| SPI MISO | PA6 |
| SPI MOSI | PA7 |
| NSS (CS) | PA4 |
| BUSY | PB1 |
| DIO1 (IRQ) | PB2 |
| NRESET | PB0 |

## Параметри радіо

| Параметр | Значення |
|----------|---------|
| Частота | 2450 МГц |
| Spreading Factor | SF6 |
| Bandwidth | 800 кГц |
| Coding Rate | 4/6 |
| Потужність TX | +13 дБм |
| CRC | увімкнено (апаратний SX1280) |
| Преамбула | 6 символів |

## Структура проекту

```
Core/
  Inc/
    link.h          — логіка зв'язку для обох вузлів NodeA, NodeB
    lora_protocol.h — формат пакету LP, CRC16, Build/Parse
    radio_config.h  — параметри SX1280, Radio_Init/Radio_SetIrq
  Src/
    link.c
    lora_protocol.c
    main.c          — HAL init, вибір вузла через #define
    radio_config.c
    sx128x_hal.c    — реалізація HAL SX128x драйвера
Drivers/
  SWDR005/          — драйвер SX128x (submodule)
```

## Протокол пакету (LP)

```
[0]   MAGIC   = 0xA5
[1]   VERSION = 0x01
[2]   TYPE    = 0x01 DATA | 0x02 ACK
[3]   SEQ     = номер пакету (0..255, wrap)
[4]   SRC     = ID відправника
[5]   DST     = ID одержувача
[6]   PLEN    = довжина payload
[7..] PAYLOAD (3..100 байт)
[-2,-1] CRC16-CCITT (Kermit)
```

Мінімальний пакет — 12 байт, максимальний — 109 байт.

## Збірка

Проект використовує STM32CubeIDE. Є дві конфігурації:

| Конфігурація | Define Symbol | Роль |
|-------------|--------|------|
| `Debug_NodeA` | `-DNodeA` | Ініціатор (TX DATA → чекає ACK) |
| `Debug_NodeB` | `-DNodeB` | Відповідач (RX → TX ACK) |

Прошити кожен із двох пристроїв відповідною конфігурацією.

## Сценарій роботи

```
NodeA                           NodeB
  |                               |
  |-- DATA (seq=N) -------------> |
  |                               | (parse, validate)
  |                               | (5 ms delay)
  | <----------- ACK (seq=N) -----|
  |                               |
  | (2 s delay)                   |
  |-- DATA (seq=N+1) -----------> |
  ...
```

**NodeA** виводить у UART:

```
[TX]  seq=  0 len=15 ... ACK  RSSI=-45 SNR=10 SR=100%
[TX]  seq=  1 len=15 ... ACK_TIMEOUT
```

**NodeB** виводить у UART:

```
[RX]  seq=  0 len=15 RSSI=-45 SNR=10 pld=00 03 E8 4E 6F 64 65 41
[TX]  ACK seq=0
```
