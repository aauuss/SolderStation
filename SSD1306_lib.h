
#include <avr/io.h>

#define ADDR_OLED 0x3C          //Адрес дисплея

#define ssd1306_clear() ssd1306_fill4(0, 0, 0, 0)
#define ssd1306_fill(p) ssd1306_fill4(p, p, p, p)
#define ssd1306_fill2(p1, p2) ssd1306_fill4(p1, p2, p1, p2)

const uint8_t *ssd1306tx_font_src;
uint8_t ssd1306tx_font_char_base;


void ssd1306_start_command(void);	// Initiate transmission of command
void ssd1306_start_data(void);	// Initiate transmission of data
void ssd1306_data_byte(uint8_t);	// Transmission 1 byte of data
void ssd1306_stop(void);	// Finish transmission
void ssd1306_init(void);
void ssd1306_setpos(uint8_t x, uint8_t y);
void ssd1306_fill4(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4);
void ssd1306_img(const uint8_t *img);

void ssd1306tx_char(char ch);
void ssd1306tx_string(char *s);
void ssd1306tx_init(const uint8_t *fron_src, uint8_t char_base);
void ssd1306tx_stringxy(const uint8_t *fron_src, uint8_t x, uint8_t y, const char s[]);
void ssd1306tx_stringxy_16x32(const uint8_t *fron_src, uint8_t x, uint8_t y, const char s[]);
void ssd1306_img32x32(const uint8_t *img, uint8_t x, uint8_t y, uint8_t num);
void ssd1306_img64x32(const uint8_t *img, uint8_t x, uint8_t y, uint8_t num);
