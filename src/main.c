
#include <asf.h>

signed char a, b, c,res;

int main (void)
{
		sysclk_init();
	
	DDRD=0xFF;
//CLKPR=0x80;
//CLKPR=2;

//CLKPR|=0x08;
	while(1){
		PORTD^=1<<PD6;
		delay_ms(2000);
	}
}
