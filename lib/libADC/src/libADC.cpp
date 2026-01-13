#include "libADC.hpp"


void ADC_Init(void){
	// Uruchomienie ADC, wewnêtrzne napiecie odniesienia, tryb pojedynczej konwersji, preskaler 128, wejœcie PIN5, wynik do prawej
	ADCSRA = (1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);
	//ADEN:7 - ADC Enable (uruchomienie przetwornika)
	//ADPS2:0 - prescaler (tu 128)

	// ADMUX – ADC Multiplexer Selection Register
	ADMUX  =  (1<<REFS0);
	//REFS0:6 - Reference Selection Bits ->  Internal 1.1V Voltage Reference with external capacitor at AREF pin
}


int  ADC_conversion(){
	ADCSRA |= (1<<ADSC);		//	ADC - Start Conversion
	while(ADCSRA & (1<<ADSC)); 	//	wait for finish of conversion

	return ADC;
}
