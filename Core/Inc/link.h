#ifndef INC_LINK_H_
#define INC_LINK_H_

#include "main.h"
#include "sx128x.h"
#include "lora_protocol.h"
#include "radio_config.h"
#include <stdio.h>
#include <string.h>

void node_a_init();
void node_b_init();

void node_a_loop();
void node_b_loop();

#endif /* INC_LINK_H_ */
