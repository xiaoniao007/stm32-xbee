#ifndef __ZIG_H
#define	__ZIG_H

#include "stm32f10x.h"
#include <stdio.h>

uint16_t Tramsmit_packet(uint8_t *T_packet,uint8_t *data,uint8_t *dest_add,uint16_t datanum);
uint16_t Recieve_packet(uint8_t *R_packet,uint8_t *R_data,uint8_t *resource_add,uint16_t p_num);

#endif /* __USART1_H */
