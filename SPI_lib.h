/*
 * Cамая лучшая библиотека для программного SPI на ATMega328 и т.п.
 */


#include <avr/io.h>

#define SCK	    PC2   // ----> [SCK]	
#define CS		PC1   // ----> [CS]
#define SO		PC0   // ----> [SO]	
                      //       ATMega328 SolderStation
uint16_t lastResult;

void spi_init(void);          
void spi_byte(uint8_t byte);
uint16_t spi_read_word();
uint8_t spi_read_b(void);
