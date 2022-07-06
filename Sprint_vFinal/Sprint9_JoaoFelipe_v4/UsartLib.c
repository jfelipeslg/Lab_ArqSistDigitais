/*
* UsartLib.c
*
* Created: 01/05/2021 13:41:53
* Autor do código: Rafael Lima, Professor da UFCG
*/

#include "UsartLib.h"
#include <stdlib.h>

unsigned char pressao_arterial[8] = "HHHxLLL";
uint16_t pressao_H, pressao_L;

ISR(USART_RX_vect)
{
	static uint8_t start_stop_flag = 0, index = 0;
	char char_recebido = UDR0;
	
	switch (char_recebido)
	{
		case ';':
			start_stop_flag = 1;
			index = 0;
			break;
			
		case ':':
			start_stop_flag = 0;
			pressao_arterial[index] = '\0';
			if(Check_payload_HHHxLLL(pressao_arterial, &pressao_H, &pressao_L) == 0)
			{
				strcpy(pressao_arterial, "ERRO!");
			}
			break;
		
		default:
			if(start_stop_flag)
			{
				if(index < 7)
				{
					pressao_arterial[index++] = char_recebido;
				}
 				else
				{
					start_stop_flag = 0;
					strcpy(pressao_arterial, "ERRO!");
				}
			}
	}
}

void USART_Init(unsigned int ubrr)
{
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
	
	DDRC |= 0b00000010;
	DDRC &= 0b11111110;
}

void USART_Transmit(unsigned char data)
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

unsigned char USART_Receive(void)
{
	while(!(UCSR0A & (1<<RXC0)));
	return UDR0;
}

uint8_t Check_payload_HHHxLLL(char* payload, uint16_t *HHH, uint16_t *LLL)
{
	unsigned char Pressao_H[8], Pressao_L[8];
	char *split, *aux;
	
	aux = strdup(payload);
	split = strsep(&aux, "x");
	strcpy(Pressao_H, split);
	split = strsep (&aux, "x");
	strcpy(Pressao_L, split);
	
	*HHH = atoi(Pressao_H);
	*LLL = atoi(Pressao_L);
	
	if(*HHH>=0 & *HHH<=999 & *LLL>=0 & *LLL<=999)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}