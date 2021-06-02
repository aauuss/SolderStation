#include "TWI_lib.h"

#ifndef F_CPU
#define F_CPU 12000000UL
#endif

#include <stdlib.h>
#include <avr/io.h>
#include <stdio.h>

#include <util/delay.h>

void twi_init(void) {
  TWBR = 0x34; //для 100кГц при частоте мк 12МГц
}

void twi_start(void) {
 TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
}

void twi_stop(void) {
 TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

void twi_byte(uint8_t byte) {
 TWDR = byte;//запишем байт в регистр данных
 TWCR = (1<<TWINT)|(1<<TWEN);//включим передачу байта
 while (!(TWCR & (1<<TWINT)));//подождем пока установится TWIN
}

uint8_t twi_read(void) {
 TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);//включим прием данных
 while(!(TWCR & (1<<TWINT)));//подождем пока установится TWIN
 return TWDR;
}

uint8_t twi_readACK(void) {         // то же, что и просто read
 TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);//включим прием данных
 while(!(TWCR & (1<<TWINT)));//подождем пока установится TWIN
 return TWDR;
}

uint8_t twi_readNAK(void) {
 TWCR = (1<<TWINT)|(1<<TWEN);//включим прием данных
 while(!(TWCR&(1<<TWINT)));//подождем пока установится TWIN
 return TWDR;
}
