/*
 * GccApplication1.c
 *
 * Created: 10/07/2021 11:02:53
 * Author : Isac Gomes de Almeida Silva - 118210312
 */ 

#define F_CPU 16000000UL 
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "PCD8544\nokia5110.h"
void display();
void semafaro();
void luminaria();
void USART_Transmit(uint8_t tempo_transmit);

uint8_t selecao = 0, modo = 1;
uint8_t temp_verde = 1, temp_verme = 1, temp_amare = 1, verde_auto =1, verme_auto=1,verme_escravo=0,verde_escravo=0;
uint32_t tempo_1ms = 0;
uint8_t flag_verde = 0, flag_verme = 0, flag_amare = 0, flag_freq=0, flag_lumi=0, flag_aux1 = 0;
uint32_t carros_min=0;
uint16_t lux=0;
uint8_t chuva=0;
uint8_t cont_c5s=0;  // variável responsável por armazenar a quantidade de carros que passam no intervalo de 5 segundos


ISR(INT0_vect) // Acionada caso o botão de acréscimo de tempo for acionado
{
	if(selecao == 0){ // Verifica se a seta esta localizada na seleção de modo  e muda o seu estado
		modo++;
		if(modo == 3){
			modo = 1;
		}
	}
	else if(selecao == 1){ // Verifica em que sinal esta localizado a seta e incrementa
		temp_verde++;
		if(temp_verde == 10){ // Verifica se chegou ao seu valor máximo
			temp_verde = 1;
		}
	}
	else if(selecao == 2){ // Verifica em que sinal esta localizado a seta e incrementa
		temp_verme++;
		if(temp_verme == 10){ // Verifica se chegou ao seu valor máximo
			temp_verme = 1;
		}
	}
	else if(selecao == 3){ // Verifica em que sinal esta localizado a seta e incrementa
		temp_amare++;
		if(temp_amare == 10){ // Verifica se chegou ao seu valor máximo
			temp_amare = 1;
		}
	}
	display(); // Atualiza o display em tempo real
}

ISR(INT1_vect) // Acionada caso o botão de decréscimo de tempo for acionado
{
	if(selecao == 0){ // Verifica se a seta esta localizada na seleção de modo e muda o seu estado
		modo--;
		if(modo == 0){
			modo = 2;
		}
	}
	else if(selecao == 1){ // Verifica em que sinal esta localizado a seta e decrementa
		temp_verde--;
		if(temp_verde == 0){ // Verifica se chegou ao seu valor mínimo
			temp_verde = 1;
		}
	}
	else if(selecao == 2){ // Verifica em que sinal esta localizado a seta e decrementa
		temp_verme--;
		if(temp_verme == 0){ // Verifica se chegou ao seu valor mínimo
			temp_verme = 1;
		}
	}
	else if(selecao == 3){ // Verifica em que sinal esta localizado a seta e decrementa
		temp_amare--;
		if(temp_amare == 0){ // Verifica se chegou ao seu valor mínimo
			temp_amare = 1;
		}
	}
	display(); // Atualiza o display em tempo real
}

ISR(PCINT2_vect) // Acionada caso o botão de seleção for acionado
{
	if(!(PIND &(1<<4)) &&  modo==1){ // Filtra apenas o acionamento da borda de descida e verifica se esta no modo manual
			selecao++; // Intrementa variável responsável por posicionar a seta
			display(); // Atualiza o display
	}	
}

ISR(PCINT0_vect) // Acionada a cada borda do gerador de pulso
{
	if(!(PINB &(1<<6))) // Filtra apenas o acionamento da borda de descida
		cont_c5s++; 
}

ISR(TIMER0_COMPA_vect) // Interrupção do TC0 a cada 1ms
{
	static uint8_t cont=4;	
	uint8_t aux2;
	tempo_1ms++;
	
	if(!(tempo_1ms % 5000)){ // A cada 5s atualiza a estimativa de carros por minuto
		carros_min = cont_c5s/5*60;
		verde_auto= carros_min*5/300+1;
		verme_auto= 9-5*carros_min/300;
		cont_c5s=0;
	}	
		
	if(!(tempo_1ms % 500)) // Ativa a flag a cada 500ms
		flag_lumi=1;
		
	if(!(tempo_1ms%(temp_amare*1000))){ // Ativa a flag para o sinal amarelo (tempo do sinal amarelo)
		flag_amare=1;
	}
	
	if(modo==1){ // Quando esta no modo manual
		verde_escravo = temp_verme-temp_amare;
		verme_escravo = temp_verde+temp_amare;
		
		if(!(tempo_1ms%(temp_verde*250))) // Ativa a flag para cada led do sinal verde (tempo de cada led do verde)
			flag_verde=1;
		if(!(tempo_1ms%(temp_verme*250))) // Ativa a flag para cada led do sinal vermelho (tempo de cada led do vermelho)
			flag_verme=1;
	}
	
	else{ // Quando esta no modo automatico
		verde_escravo = verme_auto-temp_amare;
		verme_escravo = verde_auto+temp_amare;
		
		if(!(tempo_1ms%(verde_auto*250))) // Ativa a flag para cada led do sinal verde (tempo de cada led do verde)
			flag_verde=1;
		if(!(tempo_1ms%(verme_auto*250))) // Ativa a flag para cada led do sinal vermelho (tempo de cada led do vermelho)
			flag_verme=1;
	}
	

	if(cont<=4){ // Transfere o tempo de cada led do vermelho
		if(!(tempo_1ms%(verme_escravo*250))){ 
				if(cont!=0)
					USART_Transmit(cont);
				cont--;			
				if(cont==0){
					cont=9;
					USART_Transmit(cont);
				}
		}
				
	}
		if(cont>4 && cont<=9){ // Transfere o tempo de cada led do verde
			if(!(tempo_1ms%(verde_escravo*250))){ 
				if(cont>5)
					USART_Transmit(cont);
			
				if(cont==5){ // Liga o amarelo
					USART_Transmit(cont);
					cont=11;
				}
				cont--;
			}
			
		}
		if(cont==10){ // Passa o tempo do amarelo e liga o vermelho
			if(!(tempo_1ms%(temp_amare*1000))){ 
				USART_Transmit(4);
				cont=3;
				}		
		}
}

	int main(void)
{
	DDRB &= 0b01111111; // Habilita os pinos PB0, PB1, PB2, PB3 como saídas (leds do semáfaro) e PB6 como entrada (gerador de pulso)
	DDRD |= 0b01100001; // Habilita o pino PD5 como saída (luminária)
	DDRD &= 0b011100011; // Habilita os pinos PD0, PD2, PD3 e PD4 como entradas (chave pedestres e botões seleção)
	DDRC &= 0b1111100; // Habilita os pinos PC0 e PC1 como entradas
	PORTD |= 0b11011100; // Habilita o resistor de pull-up dos pinos PD0, PD2, PD3 e PD4
	PORTB |= 0b10000000; // Habilita o resistor de pull-up do pino PB6
	
	TCCR0A = 0b10100011; // PWM não invertido nos pinos OC0A e OC0B
	TCCR0B = 0b00000011; // Liga o TC0 com prescaler = 64;
	OCR0A = 249; // Ajusta o comparador para o TC0 contar até 249
	OCR0B=0; // Inicia com a luminária apagada
	TIMSK0 = 0b00000010; // Habilita a interrupção na igualdade de comparação com OCR0A. A interrupção ocorre a cada 1ms = (64*(249+1))/16MHz

	EICRA = 0b00001010; // Configura as carectirísticas das interrupções INT0 e INT1 para ser acionada a cada borda de descida
	EIMSK = 0b00000011; // Habilita das interrupções INT0 e INT1
	PCICR = 0b00000101; // Habilita as interrupções PCINT1(port C) e PCINT2(port D) 
	PCMSK2 = 0b00010001; // Habilita a interrupção individual do pino PD4
	PCMSK0 = 0b01000000; // Habilita a interrupção individual do pino PC6
	
	ADMUX = 0b01000000; // VCC como referência, canal 0
	ADCSRA = 0b11100111; // Habilita o AD com modo de conversão contínua e prescaler = 128
	ADCSRB = 0b00000000; // Modo de conversão contínua
	
	UBRR0H = (unsigned char)(MYUBRR>>8);
	UBRR0L = (unsigned char)(MYUBRR);
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
	
	sei(); // Habilita a chave geral de interrupções
	
	nokia_lcd_init(); // Liga o display
	display(); // Inicializa o display
	
	/* Replace with your application code */
    while (1) 
    {
	semafaro(); // Executa o semáfaro
	luminaria(); // Executa a luminária
	}
}

void display(){ // Atualiza e mostra display
	
	unsigned char verde_string[2];
	unsigned char verme_string[2];
	unsigned char amare_string[2];
	unsigned char carros_string[4];
	unsigned char verdeauto_string[2];
	unsigned char vermeauto_string[2];
	unsigned char lux_string[4];
	unsigned char chuva_string[3];
	
	// Converte o valor da variável inteira para string para mostrar no display
	sprintf(verdeauto_string,"%u",verde_auto);
	sprintf(vermeauto_string,"%u",verme_auto);
	sprintf(carros_string,"%u",carros_min);
	sprintf(verde_string,"%u",temp_verde); 
	sprintf(verme_string,"%u",temp_verme);
	sprintf(amare_string,"%u",temp_amare);	
	sprintf(lux_string,"%u",lux);		
	sprintf(chuva_string,"%u",chuva);
	
	nokia_lcd_clear();
	if(selecao > 3){ // Verifica se chegou a última opção de seleção
		selecao = 0;
	}
	
	if(modo == 1){ // Caso esteja no modo manual
		nokia_lcd_set_cursor(27,7);
		nokia_lcd_write_string("M",1);
		nokia_lcd_set_cursor(27,17);
		nokia_lcd_write_string(verde_string,1);
		nokia_lcd_set_cursor(27,27);
		nokia_lcd_write_string(verme_string,1);
	}
	else if(modo==2){ // Caso esteja no modo automatico
		nokia_lcd_set_cursor(27,7);
		nokia_lcd_write_string("A",1);
		nokia_lcd_set_cursor(27,17);
		nokia_lcd_write_string(verdeauto_string,1);
		nokia_lcd_set_cursor(27,27);
		nokia_lcd_write_string(vermeauto_string,1);	
	}
		
	nokia_lcd_set_cursor(0,7);
	nokia_lcd_write_string("Modo",1);
	nokia_lcd_set_cursor(0,17);
	nokia_lcd_write_string("T.Vd",1);
	nokia_lcd_set_cursor(33,17);
	nokia_lcd_write_string("s",1);
	nokia_lcd_set_cursor(0,27);
	nokia_lcd_write_string("T.Vm",1);
	nokia_lcd_set_cursor(33,27);
	nokia_lcd_write_string("s",1);
	nokia_lcd_set_cursor(0,37);
	nokia_lcd_write_string("T.Am",1);
	nokia_lcd_set_cursor(27,37);
	nokia_lcd_write_string(amare_string,1);
	nokia_lcd_set_cursor(33,37);
	nokia_lcd_write_string("s",1);
	
	nokia_lcd_set_cursor(48,0);
	nokia_lcd_write_string(lux_string,1);
	nokia_lcd_set_cursor(67,0);
	nokia_lcd_write_string("lux",1);
	
	nokia_lcd_set_cursor(48,12);
	nokia_lcd_write_string(chuva_string,1);
	nokia_lcd_set_cursor(61,12);
	nokia_lcd_write_string("mm/h",1);
	
	nokia_lcd_set_cursor(48,26);
	nokia_lcd_write_string(carros_string,2);
	nokia_lcd_set_cursor(50,41);
	nokia_lcd_write_string("c/min",1);

	nokia_lcd_set_cursor(43,0);
	nokia_lcd_write_string("|",1);
	nokia_lcd_set_cursor(43,7);
	nokia_lcd_write_string("|",1);
	nokia_lcd_set_cursor(43,14);
	nokia_lcd_write_string("|",1);
	nokia_lcd_set_cursor(43,21);
	nokia_lcd_write_string("|",1);
	nokia_lcd_set_cursor(43,28);
	nokia_lcd_write_string("|",1);
	nokia_lcd_set_cursor(43,35);
	nokia_lcd_write_string("|",1);
	nokia_lcd_set_cursor(43,42);
	nokia_lcd_write_string("|",1);
	
	if(selecao == 0){ // Posiciona o botão de seleção na escolha do modo
		nokia_lcd_set_cursor(39,7);
		nokia_lcd_write_string("<",1);
	}
	
	else if(selecao == 1){ // Posiciona o botão de seleção no sinal verde
		nokia_lcd_set_cursor(39,17);
		nokia_lcd_write_string("<",1);
	}
	
	else if(selecao == 2){ // Posiciona o botão de seleção no sinal vermelho
		nokia_lcd_set_cursor(39,27);
		nokia_lcd_write_string("<",1);	
	}
	
	else if(selecao == 3){ // Posiciona o botão de seleção no sinal amarelo
		nokia_lcd_set_cursor(39,37);
		nokia_lcd_write_string("<",1);
	}
	nokia_lcd_render();
}

void semafaro(){
	static uint8_t num1=9, num2=0;
	static uint8_t estado=0, estado2=0; // Variável responsável por informar qual etapa do semáfaro deve ser executada
	
	if(estado==0){ // Condição para ligar todos os leds do sinal verde
		PORTB = num1;
		estado=1;
		flag_verde=0;
		num2=5;
	}
	if(flag_verde==1 && estado==1){ // Condição para ir desligando um a um os leds do sinal verde
		num1--;
		PORTB = num1;
		
		if(num1==5){ // Quando chegar no final do sinal verde liga o sinal amarelo
			PORTB = 0b00000101;
			estado=2;
			flag_amare=0;
		}
		flag_verde=0; 
	}
	
	if(flag_amare==1 && estado==2){ // Como ligamos o sinal amarelo lá atrás esta condição serve para após decorrer o tempo do amarelo acender o vermelho
		PORTB = 0b00000100;
		estado=3;
		flag_amare=0;
		flag_verme=0;
	}
	
	if(flag_verme==1 && estado==3){ // Condição para ir desligando um a um os leds do sinal vermelho
		num1--;
		PORTB = num1;
		if(num1==0){ // Quando chegar no final do sinal vermelho encaminha para a condição que liga o sinal verde
			estado=1;
			num1=0b00001001;
			PORTB=num1;
			flag_verde=0;
		}
	flag_verme=0;
	}	
}

void luminaria(){
	static uint8_t cont=0; // varivável responsável por mudar o canal
	static uint8_t pedestre=0;
		
	if(!(PIND &(1<<7))) // Caso detecte fluxo de pedestres (botão ativado)	
		pedestre=1;
		
	if(flag_lumi==1){
		if(cont==0){ // Pra atualizar o valor da intensidade da luz em LUX
				lux=1023000/ADC - 1000;
				ADMUX = 0b01000001; // Continua com refeência VCC e muda para o canal 1
				cont++;
		}
		else{ // Pra atualizar o valor da intensidade da chuva em mm/hora
			chuva= ((float)50/1023*ADC+5);
			ADMUX = 0b01000000; // Continua com referência VCC e volta para o canal 0
			cont--;
		}
	}

	if((lux<300 && pedestre==1)||(lux<300 && carros_min>0)||(chuva>10 && pedestre==1)||(chuva>10 && carros_min>0)){ // Caso luz natural menor que 300lux ou chuva maior que 10mm/h e seja identificado fluxo de pedestres ou de carros liga a luminária em 100%
		OCR0B=255;
		pedestre=0;
	}
		
	else if((lux<300 && pedestre==0)||(lux<300 && carros_min==0)||(chuva>10 && pedestre==0)||(chuva>10 && carros_min==0)){ // Caso luz natural menor que 300lux ou chuva maior que 10mm/h e não seja identificado fluxo de pedestres ou de carros liga a luminária em 30%
		OCR0B=77;
		pedestre=0;
	}
	
	else // Caso luz natural maior ou igual a 300lux e chuva menor ou igual a 10mm/h luminária desligada
		OCR0B=0;
	
	flag_lumi=0;
	display();
}


void USART_Transmit(uint8_t tempo_transmit){ // Função de transferência
	while(!(UCSR0A &(1<<UDRE0)));
	UDR0 = tempo_transmit;
}



//uint8_t LUT[10] = {0b00000000,0b00000001,0b00000010,0b00000011,0b00000100,0b00000101,0b00000110,0b0000111,0b00001000,0b00001001};	
	//Port = LUT[cont]