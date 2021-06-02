/*
 * Cамая лучшая библиотека для аппаратного I2C на ATMega328 и т.п.
 */


#include <avr/io.h>

//#define SCL		PC5   // ----> [SCL]	
//#define SDA		PC4   // ----> [SDA]	
                      //       ATMega328

void twi_init(void);
void twi_start(void);         
void twi_stop(void);          
void twi_byte(uint8_t byte);
uint8_t twi_read(void);
uint8_t twi_readACK(void);
uint8_t twi_readNAK(void);
