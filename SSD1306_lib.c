#include <avr/io.h>
#include "TWI_lib.h"
#include "SSD1306_lib.h"
#include <avr/pgmspace.h>

const uint8_t ssd1306_init_sequence [] PROGMEM = {	// Initialization Sequence
	0xAE,			// Display OFF (sleep mode)
	0x20, 0x00,		// Set Memory Addressing Mode
	//				// 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
	//				// 10=Page Addressing Mode (RESET); 11=Invalid
	0xB0,			// Set Page Start Address for Page Addressing Mode, 0-7
	0xC8,			// Set COM Output Scan Direction
	0x00,			// ---set low column address
	0x10,			// ---set high column address
	0x40,			// --set start line address
	0x81, 0xFF,		// Set contrast control register
	0xA1,			// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
	0xA6,			// Set display mode. A6=Normal; A7=Inverse
	0xA8, 0xFF,		// Set multiplex ratio(1 to 64)
	0xA4,			// Output RAM to Display
	//				// 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
	0xD3, 0x00,		// Set display offset. 00 = no offset
	0xD5,			// --set display clock divide ratio/oscillator frequency
	0xF0,			// --set divide ratio
	0xD9, 0x22,		// Set pre-charge period
	0xDA, 0x12,		// Set com pins hardware configuration
	0xDB,			// --set vcomh
	0x20,			// 0x20,0.77xVcc
	0x8D, 0x14,		// Set DC-DC enable
	0xAF			// Display ON in normal mode
};

void ssd1306_start_command(void) {
	twi_start();
	twi_byte((ADDR_OLED << 1));	// Slave address: R/W(SA0)=0 - write
	twi_byte(0x00);			// Control byte: D/C=0 - write command
}

void ssd1306_start_data(void) {
	twi_start();
	twi_byte((ADDR_OLED << 1));	// Slave address, R/W(SA0)=0 - write
	twi_byte(0x40);			// Control byte: D/C=1 - write data
}

void ssd1306_data_byte(uint8_t b) {
	twi_byte(b);
}

void ssd1306_stop(void) {
	twi_stop();
}

void ssd1306_init(void) {
	ssd1306_start_command();	// Initiate transmission of command
	for (uint8_t i = 0; i < sizeof (ssd1306_init_sequence); i++) {
		ssd1306_data_byte(pgm_read_byte(&ssd1306_init_sequence[i]));	// Send the command out
    }
	ssd1306_stop();	// Finish transmission
}

void ssd1306_setpos(uint8_t x, uint8_t y) {
	ssd1306_start_command();
	ssd1306_data_byte(0xb0 | (y & 0x07));	// Set page start address
	ssd1306_data_byte(x & 0x0f);			// Set the lower nibble of the column start address
	ssd1306_data_byte(0x10 | (x >> 4));		// Set the higher nibble of the column start address
	ssd1306_stop();	// Finish transmission
}

void ssd1306_fill4(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
/*  ssd1306_setpos(0, 0);
	ssd1306_start_data();	// Initiate transmission of data
	for (uint16_t i = 0; i <= 128 * 8 / 4 + 128; i++) {
		ssd1306_data_byte(p1);
		ssd1306_data_byte(p2);
		ssd1306_data_byte(p3);
		ssd1306_data_byte(p4);
	}
	ssd1306_stop();	// Finish transmission
    ssd1306_setpos(0, 0);
*/
    for (uint16_t y = 0; y < 8; y++){
        ssd1306_setpos(0, y);   
        ssd1306_start_data(); 
        for (uint16_t i = 0; i <= 32; i++) {
            ssd1306_data_byte(p1);
    		ssd1306_data_byte(p2);
    		ssd1306_data_byte(p3);
    		ssd1306_data_byte(p4);
        }    
        ssd1306_stop();
    }
}

void ssd1306_img(const uint8_t *img) {
    for (uint16_t y = 0; y < 8; y++){
        ssd1306_setpos(0, y);   
        ssd1306_start_data(); 
        for (uint16_t i = 0; i <= 128; i++) {
            ssd1306_data_byte(pgm_read_byte(&img[y*128+i]));
        }    
        ssd1306_stop();
    }
}

void ssd1306_img64x32(const uint8_t *img, uint8_t x, uint8_t y, uint8_t num) {
    for (uint16_t i = 0; i < 4; i++){
        ssd1306_setpos(x, y+i);   
        ssd1306_start_data(); 
        for (uint16_t j = 0; j <= 64; j++) {
            ssd1306_data_byte(pgm_read_byte(&img[i*64+j + 256*num]));
        }    
        ssd1306_stop();
    }
}

void ssd1306_img32x32(const uint8_t *img, uint8_t x, uint8_t y, uint8_t num) {
    for (uint16_t i = 0; i < 4; i++){
        ssd1306_setpos(x, y+i);   
        ssd1306_start_data(); 
        for (uint16_t j = 0; j <= 32; j++) {
            ssd1306_data_byte(pgm_read_byte(&img[i*32+j + 128*num]));
        }    
        ssd1306_stop();
    }
}

void ssd1306tx_char(char ch) {
	uint8_t c = ch - 32; // Convert ASCII code to font data index.
	ssd1306_start_data();
	for (uint8_t i = 0; i < 6; i++) {
		ssd1306_data_byte(pgm_read_byte(&ssd1306tx_font_src[c * 6 + i]));	// TODO: Optimize this.
	}
	ssd1306_stop();
}

void ssd1306tx_string(char *s) {
	while (*s) {
		ssd1306tx_char(*s++);
	}
}

void ssd1306tx_init(const uint8_t *fron_src, uint8_t char_base) {
	ssd1306tx_font_src = fron_src;
	ssd1306tx_font_char_base = char_base;
}

void ssd1306tx_stringxy(const uint8_t *fron_src, uint8_t x, uint8_t y, const char s[]) {
	uint8_t ch, j = 0;
	while (s[j] != '\0') {
		ch = s[j] - 32; // Convert ASCII code to font data index.
		if (x > 120) {
			x = 0;    // Go to the next line.
			y+=2;
		}
		ssd1306_setpos(x, y);
		ssd1306_start_data();
		for (uint8_t i = 0; i < 8; i++) {
			ssd1306_data_byte(pgm_read_byte(&fron_src[ch * 16 + i]));
		}
		ssd1306_stop();
		ssd1306_setpos(x, y + 1);
		ssd1306_start_data();
		for (uint8_t i = 0; i < 8; i++) {
			ssd1306_data_byte(pgm_read_byte(&fron_src[ch * 16 + i + 8]));
		}
		ssd1306_stop();
		x += 8;
		j++;
	}
}

void ssd1306tx_stringxy_16x32(const uint8_t *fron_src, uint8_t x, uint8_t y, const char s[]) {
	uint16_t ch, c = 0;
	while (s[c] != '\0') {
        if (s[c] == ' '){ 
          ch = 10;}
        else {
		  ch = s[c] - 48;} // Convert ASCII code to font data index.
        for (uint8_t j = 0; j < 4; j++) {
		    ssd1306_setpos(x, y+j);
		    ssd1306_start_data();
		    for (uint8_t i = 0; i < 16; i++) {
			    ssd1306_data_byte(pgm_read_byte(&fron_src[(ch * 64) + (j * 16) + i]));
		    }
		    ssd1306_stop();		    		    
	    }
    c++;
    x += 18;
    }
}
