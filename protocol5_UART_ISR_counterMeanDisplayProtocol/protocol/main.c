#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "main.h"

const uint8_t myID = '8'; //ID ����������

//������ �� ������� �� �������� UBRR = ( F_CPU /( Baud * 16 ) ) - 1
//�� � ����������� � ������� �������, � �� � ������������� ������� �����
#define CONSOLE_BAUDRATE 19200
#define CONSOLE_BAUD_PRESCALE (((F_CPU) + 8UL * (CONSOLE_BAUDRATE)) / (16UL * (CONSOLE_BAUDRATE)) - 1UL)

#define a (1<<PB7)
#define b (1<<PB6)
#define c (1<<PB5)
#define d (1<<PB4)
#define e (1<<PB3)
#define f (1<<PB2)
#define g (1<<PB1)
#define dp (1<<PB0)

volatile uint8_t packet[5];
volatile bool PackWas;
/*
  Name  : CRC-8
  Poly  : 0x31    x^8 + x^5 + x^4 + 1
  Init  : 0xFF
  Revert: false
  XorOut: 0x00
  Check : 0xF7 ("123456789")
  MaxLen: 15 ����(127 ���) - �����������
    ���������, �������, ������� � ���� �������� ������
*/
uint8_t Crc8(uint8_t *pcBlock, uint8_t len)
{
    uint8_t crc = 0xFF;
    uint8_t i;
    while(len--){
        crc ^= *pcBlock++;
        for(i= 0;i<8;i++) 
			crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }

    return crc;
}

volatile float resF; //���������� �������

ISR(USART_RX_vect)
{
	static uint8_t pack_i;
	//PORTD ^=(1<<PD7);
	if(pack_i < 5){
		packet[pack_i] = UDR0;
		pack_i++;
	}
	if(pack_i >=5){
		PackWas = true;
		pack_i=0;
	}
}

void uart_Init(){
	UBRR0H = CONSOLE_BAUD_PRESCALE >> 8; //��������� ������� ���� 16 ������� �������� UBRR0
	UBRR0L = (uint8_t)CONSOLE_BAUD_PRESCALE; //��������� ������� ���� 16 ������� �������� UBRR0
	
	UCSR0C = (0 << UMSEL01) | (0 << UMSEL00)  /*  */
		| (1 << UPM01) | (0 << UPM00)    /* Enabled, Even Parity */
		| 0 << USBS0                     /* USART Stop Bit Select: disabled */
		| (1 << UCSZ01) | (1 << UCSZ00); /* 8-bit */

	UCSR0B =  1 << RXEN0   /* Receiver Enable: enabled */
	    | 1 << RXCIE0    /* RX Complete Interrupt Enable: enabled */
		| 1 << TXEN0;   /* Transmitter Enable: enabled */
}

// ������� ������ ������� �� ��, ������������ printf ��� �������
static int my_putchar(char sym, FILE *stream){
	PORTD |= (1<<PD2); //rs485 �� ����������!
	UDR0 = sym;
	while(!(UCSR0A & (1 << UDRE0))); //������� ����������� ��������� ����� ���� �� UART0
	_delay_ms(1);
	return 0;
}

uint8_t my_getchar_OUT0(){
	PORTD &= ~(1<<PD2); //rs485 �� �������!
	while (!(UCSR0A & (1<<RXC0))); //oo �������� ����� �� UART0
	return UDR0; //�������� �������, ������ ������
}

void ADC_init(){
	ADMUX = (0x07 << MUX0); /* ������������ ��� ADC7 */

	ADCSRA = (1 << ADEN)        /* ADC: �������� ������ */
		| (0x03 << ADPS0); /* ������� ��� ������ F_CPU/8 */
}

 //�� TIMER0_OVF_vect �� TIMER0_OVF_vect �������� 1/(8000000/1024)*2^8=0.032768 �
ISR(TIMER0_OVF_vect)
{
	//TCNT1 �������� ���-�� ��������, ������� ��������� �� 0.032768 �
	resF = TCNT1/0.032768;
	TCNT1 = 0; //�������� ���� �������� (������) �������� �������
}

//16-������ ������ 1. ������� ���������� �������� �������� ������� �� ������ T1 (PD5)
void TIMER_1_init_NORMAL(){
	TCCR1B = (1 << CS12) | (1 << CS11) | (0 << CS10); /*������� �������� (�����) ����� �� ������ T1*/
}

//8-������ ������ 0. TCNT0 ����������� �� 2^8 � �������� TIMER0_OVF_vect.
void TIMER_0_init(){
	TCCR0B = (1 << CS02) | (0 << CS01) | (1 << CS00); /* ������������ 1024 (������� ������� == F_CPU/1024) */
	TIMSK0 = (1 << TOIE0); /* ���������� ���������� �� ������������ */
}

//8-������ ������ 2. TCNT2 ����������� �� 2^8 � �������� TIMER2_OVF_vect.
void TIMER_2_init(){
	TCCR2B = (1 << CS22) | (0 << CS21) | (1 << CS20); /* ������������ 1024 (������� ������� == F_CPU/1024) */
	TIMSK2 = (1 << TOIE2); /* ���������� ���������� �� ������������ */
}

static FILE mystdout = FDEV_SETUP_STREAM(my_putchar, NULL, _FDEV_SETUP_RW);//��� ������ printf

char buf_wh[10];
uint8_t Slot[11];  /*������ � ������� �������� �����, ������� �����
������� ����� ���� �� ���������, ����� �� ������� �����, ������ ������
�������� �������. ����� ������� ������ �� ��������.*/
volatile uint8_t Elem[6];//� ���� ���������� �������� �����, ������� ����� ����������

uint8_t k = 0; //���������� � ������� ����������
ISR(TIMER2_OVF_vect)//��� ������������ ��������� ����������
{
	PORTC &= ~((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5)); //������� PB7, PB6, PB0
	(k == 5) ? k = 0 : k++;
	switch (k)
	{
		case 0:
		PORTC = (1 << PC0); // ����� ������
		PORTB = Elem[0];
		break;
		case 1:
		PORTC = (1 << PC1);
		PORTB = Elem[1];
		break;
		case 2:
		PORTC = (1 << PC2);
		PORTB = Elem[2];
		break;
		case 3:
		PORTC = (1 << PC3);
		PORTB = Elem[3];
		break;
		case 4:
		PORTC = (1 << PC4);
		PORTB = Elem[4];
		break;
		case 5:
		PORTC = (1 << PC5);
		PORTB = Elem[5];
		break;
	}
}

void Slot_init()
{
	Slot[0] = (a+b+c+d+e+f);
	Slot[1] = (b+c);
	Slot[2] = (a+b+g+e+d);
	Slot[3] = (a+b+g+c+d);
	Slot[4] = (f+g+b+c);       // ����� �������� �������������
	Slot[5] = (a+f+g+c+d);     // ������ ��������� ����������
	Slot[6] = (a+f+g+c+d+e);
	Slot[7] = (a+b+c);
	Slot[8] = (a+b+c+d+e+f+g);
	Slot[9] = (a+b+c+d+f+g);
	Slot[10] = dp;//�����
}


int main(void)
{
	DDRD |= (1<<PD2); //����������� ����� ��������(�����������) rs485
	
	//����� �������� �� 7�� ��������� ������ �����, �������� ���� � �� �� ���� ����� �,
	DDRB=0xFF; //����� �������� �� �� �����
	//��� ������ ����� �� ����� �������� ���� �� �����
	DDRC|=(1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5);
	
	DDRD |=(1<<PD7);
	//������������� UART � printf
	uart_Init();
	stdout =&mystdout;
	
	ADC_init();
	TIMER_0_init();
	TIMER_1_init_NORMAL();
	TIMER_2_init();
	Slot_init();
	sei(); //���������� ���������� ����� ����������� ����������
	
	sprintf(buf_wh, "%.6lu", (uint32_t)123456);//�����: 123456 ��� ������ ����� - i�� ����
	Elem[0]=Slot[buf_wh[0]-'0'];
	Elem[1]=Slot[buf_wh[1]-'0']|dp;	//dp - ��� �����
	Elem[2]=Slot[buf_wh[2]-'0'];
	Elem[3]=Slot[buf_wh[3]-'0'];
	Elem[4]=Slot[buf_wh[4]-'0'];
	Elem[5]=Slot[buf_wh[5]-'0'];
    while(1){
		
		
		if(PackWas){ //����� ��������� ������, ������ ���
			if((packet[0] == '%') && (packet[4] == '#'))
			/*if(packet[1] == myID)*/{
				//if(Crc8(packet, sizeof(packet)) == packet[3]){}
				PORTD |=(1<<PD7);
				while(1){
					printf("f=%f ", resF);
					_delay_ms(10);
					
					ADCSRA |= (1 << ADSC); //�������� ��������� ��������������
					while ((ADCSRA & (1 << ADSC))); //��������� ���������� ��������������
					float volt_f = ADCW * 2.56 / 1023.0;
					printf("v=%f ", volt_f);
				}
			}
		}
    }
}

