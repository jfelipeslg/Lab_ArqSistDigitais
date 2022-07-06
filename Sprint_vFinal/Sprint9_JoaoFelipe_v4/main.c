#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "nokia5110.h"
#include "UsartLib.h"

// Variaveis Globais
uint32_t tempo_ms = 0;

uint16_t FreqRespiracao = 30, Posicao_Servo = 0;

uint16_t SendO2 = 0;

uint16_t volume = 8;

uint16_t sel = 1;

uint16_t batimentos = 0, batimentos_min = 0;

uint16_t temp_celsius = 0, SpO2 = 0;
uint16_t flag_150ms = 0;

uint16_t luz = 0;

void animalcd(uint32_t, uint16_t , uint16_t, uint16_t, uint16_t, char *, uint16_t, uint16_t, uint16_t, uint16_t);
void Servo_Resp(uint32_t , uint16_t *, uint16_t *, uint16_t);
void Servo_O2(uint16_t *);
void bpm(uint32_t , uint16_t *,uint16_t *);
void temperatura(uint32_t , uint16_t *, uint16_t *);
void saturacao_O2(uint32_t , uint16_t *, uint16_t *);
void alarme( uint32_t, uint16_t , uint16_t );
void control_luz(uint16_t);

ISR(TIMER0_COMPA_vect)	// Contador do Temporizador [1ms]
{
	tempo_ms++;
}

ISR(PCINT0_vect)
{
	if(PINB & (1<<PINB6))
	{
		if (sel >= 5)
		{
			sel = 1;
		}
		else
		{
			sel = sel + 1;
		}
	}
}

ISR(PCINT2_vect)		// Deteção dos Batimentos Cardiacos
{
	if(PIND & (1<<PIND4))
	{
		batimentos++;
	}
}

ISR(INT0_vect)			// D2 ~ Abaixa Frequência
{
	if (sel == 2)
	{
		FreqRespiracao = FreqRespiracao - 1;
		if (FreqRespiracao < 5) {FreqRespiracao = 5;}
	}
	else if (sel == 3)
	{
		if (SendO2 <= 0) {SendO2 = 0;}
		else {SendO2 = SendO2 - 1;}
	}
	else if (sel == 4)
	{
		if(volume <= 1) {volume = 1;}
		else {volume = volume - 1;}
	}
	else if (sel == 5)
	{
		if(luz <= 0) {luz = 0;}
		else {luz = luz - 1;}
	}
	else {}
}

ISR(INT1_vect)			// D3 ~ Aumenta Frequência
{
	if (sel == 2)
	{
		FreqRespiracao = FreqRespiracao + 1;
		if (FreqRespiracao > 30) {FreqRespiracao = 30;}
	}
	
	else if (sel == 3)
	{
		if (SendO2 >= 10) {SendO2 = 10;}
		else {SendO2 = SendO2 + 1;}
	}
	else if (sel == 4)
	{
		if (volume >= 8) {volume = 8;}
		else {volume = volume + 1;}
	}
	else if (sel == 5)
	{
		if (luz >= 7) {luz = 7;}
		else {luz = luz + 1;}
	}
	else {}
}

int main(void)
{
	DDRD = 0b11100011; PORTD = 0b00011100;
	DDRB = 0b10111111; PORTB = 0b01000000;
	DDRC = 0b11111100;
	
	// Configurando ações do botão
	EIMSK  = 0b00000011;				// Habilita a interrupção externa INT0 [D2] e INT1 [D3]
	EICRA  = 0b00001010;				// Interrupção externa na borda de descida para INT0 e INT1
	PCICR  = 0b00000101;				// Habilita as Interrupções na Porta D e B
	PCMSK0 = 0b01000000;
	PCMSK2 = 0b00010000;
	
	// Configurando Timer de 1ms
	TCCR0A = 0b00000010;
	TCCR0B = 0b00000011;			// Prescaler
	OCR0A = 249;					// 16000/16MHZ => 1ms
	TIMSK0 = 0b00000010;
	
	// Configurando ADC
	ADMUX  = 0b01000000;			// Tensão interna de ref (5V), canal 0 //ADMUX = 0b01000001;		// Tensão interna de ref (5V), canal 1
	ADCSRA = 0b11100111;		// Habilita o AD, habilita interrupção, modo de conversão contínua, prescaler = 128
	ADCSRB = 0b00000000;				// Modo de conversão contínua
	DIDR0  = 0b00000000;
	
	// Configurando PWM
	ICR1 = 39999;
	TCCR1A = 0b10100010;
	TCCR1B = 0b00011010;
	OCR1A = 2000;
	OCR1B = 2000;
	
	sei();
	
	nokia_lcd_init();
	
	USART_Init(MYUBRR);
	
	while (1)
	{
		animalcd(tempo_ms, FreqRespiracao, batimentos_min, temp_celsius, SpO2, pressao_arterial, SendO2, sel, volume, luz);
		Servo_Resp(tempo_ms, &FreqRespiracao, &Posicao_Servo, volume);
		Servo_O2(&SendO2);
		bpm(tempo_ms, &batimentos, &batimentos_min);
		temperatura(tempo_ms, &temp_celsius , &flag_150ms);
		saturacao_O2(tempo_ms, &SpO2, &flag_150ms);
		alarme(tempo_ms,temp_celsius, SpO2 );
		control_luz(luz);
	}
}

void animalcd(uint32_t tempo_ms_myfun, uint16_t FreqRespiracao, uint16_t batimentos_min, uint16_t temp_celsius, uint16_t SpO2, char* pressao, uint16_t SendO2, uint16_t sel, uint16_t volume, uint16_t luz)
{
	static uint32_t tempo_ms_anterior = 0;
	
	char FreqRespiracao_LCD[5]; sprintf (FreqRespiracao_LCD, "%d", FreqRespiracao);
	char SendO2_LCD[5];  sprintf (SendO2_LCD, "%d", 10*SendO2);
	char batimentos_LCD[5]; sprintf (batimentos_LCD, "%d", batimentos_min);
	char temp_celsius_LCD[5]; sprintf (temp_celsius_LCD, "%d", temp_celsius);
	char SpO2_LCD[5]; sprintf (SpO2_LCD, "%d", SpO2);
	char Volume_LCD[5]; sprintf (Volume_LCD, "%d", volume);
	char Luz_LCD[5]; luz = luz*14.285714; sprintf (Luz_LCD, "%d", luz);
	
	if (sel == 1)
	{
		if((tempo_ms_myfun - tempo_ms_anterior) >= 200)
		{
			nokia_lcd_clear();
			nokia_lcd_write_string("   Tela 1",1);
			nokia_lcd_set_cursor(0,10);
			nokia_lcd_write_string(batimentos_LCD,1);
			nokia_lcd_write_string(" BPM",1);
			nokia_lcd_set_cursor(0,20);
			nokia_lcd_write_string(temp_celsius_LCD,1);
			nokia_lcd_write_string(" °C",1);
			nokia_lcd_set_cursor(0,30);
			nokia_lcd_write_string(SpO2_LCD,1);
			nokia_lcd_write_string(" %",1);
			nokia_lcd_set_cursor(0,40);
			nokia_lcd_write_string(pressao,1);
			nokia_lcd_write_string(" mmHg",1);
			nokia_lcd_render();
			
			tempo_ms_anterior = tempo_ms_myfun;
		}
	}
	else if (sel == 2)
	{
		if((tempo_ms_myfun - tempo_ms_anterior) >= 200)
		{
			nokia_lcd_clear();
			nokia_lcd_write_string("   Tela 2",1);
			nokia_lcd_set_cursor(0,20);
			nokia_lcd_write_string(FreqRespiracao_LCD,1);
			nokia_lcd_write_string(" Resp/Min **",1);
			nokia_lcd_set_cursor(0,30);
			nokia_lcd_write_string(SendO2_LCD,1);
			nokia_lcd_write_string("% O2",1);
			nokia_lcd_set_cursor(0,40);
			nokia_lcd_write_string(Volume_LCD,1);
			nokia_lcd_write_string(" Volume",1);
			nokia_lcd_render();
			
			tempo_ms_anterior = tempo_ms_myfun;
		}
	}
	
	else if(sel == 3)
	{
		if((tempo_ms_myfun - tempo_ms_anterior) >= 200)
		{
			nokia_lcd_clear();
			nokia_lcd_write_string("   Tela 2",1);
			nokia_lcd_set_cursor(0,20);
			nokia_lcd_write_string(FreqRespiracao_LCD,1);
			nokia_lcd_write_string(" Resp/Min",1);
			nokia_lcd_set_cursor(0,30);
			nokia_lcd_write_string(SendO2_LCD,1);
			nokia_lcd_write_string("% O2 **",1);
			nokia_lcd_set_cursor(0,40);
			nokia_lcd_write_string(Volume_LCD,1);
			nokia_lcd_write_string(" Volume",1);
			nokia_lcd_render();
			
			tempo_ms_anterior = tempo_ms_myfun;
		}
	}
	
	else if(sel == 4)
	{
		if((tempo_ms_myfun - tempo_ms_anterior) >= 200)
		{
			nokia_lcd_clear();
			nokia_lcd_write_string("   Tela 2",1);
			nokia_lcd_set_cursor(0,20);
			nokia_lcd_write_string(FreqRespiracao_LCD,1);
			nokia_lcd_write_string(" Resp/Min",1);
			nokia_lcd_set_cursor(0,30);
			nokia_lcd_write_string(SendO2_LCD,1);
			nokia_lcd_write_string("% O2",1);
			nokia_lcd_set_cursor(0,40);
			nokia_lcd_write_string(Volume_LCD,1);
			nokia_lcd_write_string(" Volume **",1);
			nokia_lcd_render();
			
			tempo_ms_anterior = tempo_ms_myfun;
		}
	}
	
	else if(sel == 5)
	{
		if((tempo_ms_myfun - tempo_ms_anterior) >= 200)
		{
			nokia_lcd_clear();
			nokia_lcd_write_string("   Conforto",1);,
			nokia_lcd_set_cursor(0,20);
			nokia_lcd_write_string(Luz_LCD,1);
			nokia_lcd_write_string(" % da Luz",1);
			nokia_lcd_render();
			
			tempo_ms_anterior = tempo_ms_myfun;
		}
	}
	
	else
	{
		if((tempo_ms_myfun - tempo_ms_anterior) >= 200)
		{
			nokia_lcd_clear();

			nokia_lcd_set_cursor(0,20);

			nokia_lcd_write_string(" ERRO!",2);

			nokia_lcd_render();
			
			tempo_ms_anterior = tempo_ms_myfun;
		}
	}
}

void Servo_Resp(uint32_t tempo_ms_myfun, uint16_t *FreqRespiracao, uint16_t *Posicao_Servo, uint16_t vol)
{
	static uint32_t tempo_ms_anterior = 0;
	static uint32_t volume = 8;
	
	uint16_t i = 0;
	uint16_t servo[17];
	
	float seg_resp, seg_led_ms;
	
	for (i = 0; i < volume + 1; i++)		 { servo[i] = 2000 + (250 * i); }
	for (i = volume + 1; i < 2*volume ; i++) { servo[i] = servo[i - 1] - 250; }
	
	seg_resp = 60/ *FreqRespiracao;
	seg_led_ms = seg_resp*1000/(2*volume);
	
	if((tempo_ms_myfun - tempo_ms_anterior) >= seg_led_ms)
	{
		OCR1A = servo[*Posicao_Servo];
		*Posicao_Servo = *Posicao_Servo + 1;
		if (*Posicao_Servo == (2*volume))
		{
			*Posicao_Servo = 0;
			
			// O volume da respiração é trocado apenas após completar o ciclo, a fim de não inteferir de maneira descontrolada na respiração do paciente.
			volume = vol;
		}
		
		if (OCR1A == 2000) {PORTD |= 0b01000000;}
		else			   {PORTD &= 0b10111111;}
		
		tempo_ms_anterior = tempo_ms_myfun;
	}
}

void Servo_O2(uint16_t *SendO2)
{
	
	uint16_t servo[12] = {2000, 2200, 2400, 2600, 2800, 3000, 3200, 3400, 3600, 3800, 4000};
	
	OCR1B = servo[*SendO2];
}

void bpm(uint32_t tempo_ms_myfun, uint16_t *batimentos, uint16_t *batimentos_min)
{
	static uint32_t tempo_ms_anterior = 0;

	if((tempo_ms_myfun - tempo_ms_anterior) >= 3000)
	{
		*batimentos_min = *batimentos;
		*batimentos_min = 20*(*batimentos_min);
		*batimentos = 0;
		
		tempo_ms_anterior = tempo_ms_myfun;
	}
}

void temperatura(uint32_t tempo_ms_myfun, uint16_t *temp_celsius, uint16_t *flag_150ms)
{
	static uint32_t tempo_ms_anterior = 0;
	
	if((tempo_ms_myfun - tempo_ms_anterior) >= 150)
	{
		if (*flag_150ms == 0)
		{
			*temp_celsius = 0.049*ADC + 10.01039;	  // Conversão de B para Temperatura com https://mycurvefit.com/ | Pontos: (410,30),(511,35),(614,40),(717,45)
			if (*temp_celsius > 45) {*temp_celsius = 45;}
			if (*temp_celsius < 30) {*temp_celsius = 30;}
			ADMUX  = 0b01000001;
			*flag_150ms = 1;
		}
		
		tempo_ms_anterior = tempo_ms_myfun;
		
	}
}

void saturacao_O2(uint32_t tempo_ms_myfun, uint16_t *SpO2, uint16_t *flag_150ms)
{
	static uint32_t tempo_ms_anterior = 0;
	
	if((tempo_ms_myfun - tempo_ms_anterior) >= 300)
	{
		if (*flag_150ms == 1)
		{
			*SpO2 = 0.123*ADC + 0.02598703;	  // Conversão de B para Temperatura com https://mycurvefit.com/ | Pontos: (410,50),(511,62.5),(614,75),(717,87.5)
			if (*SpO2 > 100) {*SpO2 = 100;}
			ADMUX  = 0b01000000;
			*flag_150ms = 0;
		}

		tempo_ms_anterior = tempo_ms_myfun;
		
	}
}

void control_luz(uint16_t luz)
{
	uint16_t aux1 = luz;
	
	if (aux1 == 0)
	{
		PORTD |= 0b00100011;
		PORTD &= 0b11011100;
	}
	else if (aux1 == 1)
	{
		PORTD |= 0b00100011;
		PORTD &= 0b11011101;
	}
	else if (aux1 == 2)
	{
		PORTD |= 0b00100011;
		PORTD &= 0b11011110;
	}
	else if (aux1 == 3)
	{
		PORTD |= 0b00100011;
		PORTD &= 0b11011111;
	}
	else if (aux1 == 4)
	{
		PORTD |= 0b00100011;
		PORTD &= 0b11111100;
	}
	else if (aux1 == 5)
	{
		PORTD |= 0b00100011;
		PORTD &= 0b11111101;
	}
	else if (aux1 == 6)
	{
		PORTD |= 0b00100011;
		PORTD &= 0b11111110;
	}
	else if (aux1 == 7)
	{
		PORTD |= 0b00100011;
		PORTD &= 0b11111111;
	}
	else {}
}

void alarme( uint32_t tempo_ms_myfun, uint16_t temp_celsius, uint16_t SpO2)
{
	static uint32_t tempo_ms_anterior = 0;
	
	
	if( (SpO2 < 60) | (temp_celsius < 35) | (temp_celsius > 41) )
	{
		PORTD |= 0b10000000;
		
		if((tempo_ms_myfun - tempo_ms_anterior) >= 3000)
		{
			USART_Transmit('P');
			USART_Transmit('A');
			USART_Transmit('C');
			USART_Transmit('I');
			USART_Transmit('E');
			USART_Transmit('N');
			USART_Transmit('T');
			USART_Transmit('E');
			USART_Transmit(' ');
			USART_Transmit('E');
			USART_Transmit('M');
			USART_Transmit(' ');
			USART_Transmit('P');
			USART_Transmit('E');
			USART_Transmit('R');
			USART_Transmit('I');
			USART_Transmit('G');
			USART_Transmit('O');
			USART_Transmit('!');
			USART_Transmit(' ');
			
			if (SpO2 < 60)
			{
				USART_Transmit('S');
				USART_Transmit('P');
				USART_Transmit('O');
				USART_Transmit('2');
				USART_Transmit(' ');
				USART_Transmit('B');
				USART_Transmit('A');
				USART_Transmit('I');
				USART_Transmit('X');
				USART_Transmit('A');
				USART_Transmit('!');
				USART_Transmit(' ');

			}
			
			
			if (temp_celsius < 35)
			{
				USART_Transmit('T');
				USART_Transmit('E');
				USART_Transmit('M');
				USART_Transmit('P');
				USART_Transmit('E');
				USART_Transmit('R');
				USART_Transmit('A');
				USART_Transmit('T');
				USART_Transmit('U');
				USART_Transmit('R');
				USART_Transmit('A');
				USART_Transmit(' ');
				USART_Transmit('B');
				USART_Transmit('A');
				USART_Transmit('I');
				USART_Transmit('X');
				USART_Transmit('A');
				USART_Transmit('!');
				USART_Transmit(' ');

			}
			
			if (temp_celsius > 41)
			{
				USART_Transmit('T');
				USART_Transmit('E');
				USART_Transmit('M');
				USART_Transmit('P');
				USART_Transmit('E');
				USART_Transmit('R');
				USART_Transmit('A');
				USART_Transmit('T');
				USART_Transmit('U');
				USART_Transmit('R');
				USART_Transmit('A');
				USART_Transmit(' ');
				USART_Transmit('A');
				USART_Transmit('L');
				USART_Transmit('T');
				USART_Transmit('A');
				USART_Transmit('!');
				USART_Transmit(' ');

			}

			tempo_ms_anterior = tempo_ms_myfun;
		}
	}
	
	else
	{
		PORTD &= 0b01111111;
	}
}