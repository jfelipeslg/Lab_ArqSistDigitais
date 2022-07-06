/*
 * UsartLib.h
 *
 * Created: 01/05/2021 13:34:49
 * Autor do código: Rafael Lima, Professor da UFCG
 */ 


#ifndef USARTLIB_H_
#define USARTLIB_H_

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

void USART_Init(unsigned int ubrr);
void USART_Transmit(unsigned char data);
unsigned char USART_Receive(void);
uint8_t Check_payload_HHHxLLL(char *payload, uint16_t *HHH, uint16_t *LLL);

extern unsigned char pressao_arterial[8];
extern uint16_t pressao_H, pressao_L;

#endif /* USARTLIB_H_ */