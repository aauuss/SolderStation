#include "SPI_lib.h"

#ifndef F_CPU
#define F_CPU 12000000UL
#endif

#include <stdlib.h>
#include <avr/io.h>
#include <stdio.h>

#include <util/delay.h>

void spi_init(void) {   //настраиваем ввод/вывод как положено
DDRC |= (1 << CS) | (1 << SCK);
DDRC &= ~(1 << SO);
PORTC |= (1 << CS);
PORTC &= ~(1 << SO);
}

void spi_byte(uint8_t byte) {
    //...    
    //здесь должна быть процедура отправки байта по шине SPI
    //...
}

uint8_t spi_read_b(void) {                          //читаем 1 байт
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        data |= ((PINC >> SO) & 0x01);

        PORTC |= (1 << SCK);
        _delay_us(10);
        
        PORTC &= ~(1 << SCK);
        _delay_us(10);
    }
    return data;                
}

uint16_t spi_read_word() {             //чиатем 2 байта       
  uint16_t data;                                  //если опрашивать датчик чаще чем раз в 250мс то он перестает
  PORTC &= ~(1 << CS);                            //обновлять показания
  _delay_us(10);
  data = spi_read_b();
  data <<= 8;
  data |= spi_read_b();
  PORTC |= (1 << CS);
  if (data & 0x04) {
      lastResult = 0xFF;
      return 0xFF;
  }
  else {
      data >>= 3;
      lastResult = data;
      return data;
  }
}                                       
