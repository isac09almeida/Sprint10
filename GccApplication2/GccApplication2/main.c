/*
 * GccApplication2.c
 *
 * Created: 27/09/2021 15:27:49
 * Author : Isac
 */ 

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#include <avr/io.h>
#include <avr/interrupt.h>


ISR(USART_RX_vect){

	uint8_t cont; //Variavel que armazena o estado do semafaro
	
	cont = UDR0;
	PORTB = cont;
}



int main(void)
{
    /* Replace with your application code */
    DDRB &= 0b00001111; // Habilita os pinos PB0, PB1, PB2, PB3 como saídas (leds do semáfaro)
	
	TCCR0A = 0b10100011; // PWM não invertido nos pinos OC0A e OC0B
	TCCR0B = 0b00000011; // Liga o TC0 com prescaler = 64;
	OCR0A = 249; // Ajusta o comparador para o TC0 contar até 249
	OCR0B=0; // Inicia com a luminária apagada
	TIMSK0 = 0b00000010; // Habilita a interrupção na igualdade de comparação com OCR0A. A interrupção ocorre a cada 1ms = (64*(249+1))/16MHz
	
	//Configurações da USART
	UBRR0H = (unsigned char)(MYUBRR>>8);
	UBRR0L = (unsigned char)(MYUBRR);
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
	
	sei();
	
	while (1) {
	}
}
