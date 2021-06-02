#define F_CPU 12000000UL

#define TEH PB0
#define FAN PB2

#define ENCA PD3
#define ENCB PD4
#define SW  PD6
#define GK  PD7

/*  КОНСТАНТЫ  */
#define TEH_PWM_PERIOD 1000         //период ШИМ для ТЕНа (мс)
#define MEASUREMENT_TIME 250        //частота измерений температуры и обсчета ПИД-регулятора
#define DISPLAY_PRINT_TIME 500      //частота вывода на дисплей
#define MEASUREMENT_FILTER_CONST 4  //константа для фильтрации измерений температуры
#define SW_PRESS_TIME 3500          //длительность нажатия на энкодер для перевода в режим man/auto
#define DISPLAY_UPDATE_COUNT 5000   //количество отображений до перерисовки дсиплея
#define SHOW_SP_TIME 2000           //время показа задания при его изменении
#define DISPLAY_DATA_SWITCH_TIME 3500 //время чередования показаний в режиме MAN
#define TEMPERATURE_LIMIT 550
#define KP 1                        //коэффициенты регулятора
#define TI 1

/*  ---------  */

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <math.h>

#include "SPI_lib.h"
#include "TWI_lib.h"
#include "SSD1306_lib.h"

#include "font16x32.h"
#include "images.h"

uint32_t millis = 0;
uint16_t sec = 0;           //миллисекунды и секунды со старта. общее время = millis/1000
uint8_t state = 0; 
uint8_t mode = 1;           //режим работы регулятора 0-руч 1 -авто
uint8_t swPressed = 0;
uint8_t displayUpdateFlag = 0,//флаг для перерисовки дисплея
        spChanged = 0,      //флаг изменения задания
        showOPFlag = 0;     //флаг выбора отображать OP или температуру
int16_t speedFan = 0;       //0..100%
int16_t count = 0;          //счетчик для теста
int16_t TEH_op = 0;         //0..100%
int16_t lastTEH_op = 0;     //предыдущее значение выхода (только при mode == 0)
int16_t TEH_sp = 0;         //задание для регулятора температуры
int16_t lastTEH_sp = 0;     //предыдущее задание для детектора изменения

uint32_t TEH_PWM_lastTime = 0;  
uint32_t lastTimeTempMeasure = 0;   //последнее измерение температуры
uint32_t lastTimeDisplayPrint = 0;  //          вывод на дисплей
uint32_t lastTimeSWPressed = 0;     //время последнего нажатия на энкодер
uint32_t lastTimeSPChange = 0;      //время последнего изменения задания
uint32_t lastTimeDisplayDataSwitch = 0;      //время последнего изменения OP
uint16_t countDisplayPrint = DISPLAY_UPDATE_COUNT + 1;
double temperature;

// счетчик времени
ISR (TIMER0_COMPA_vect) {   
  millis++;               
}

// нажатие кнопки ON/OFF
ISR (INT0_vect){
 state ^= 1;
 _delay_us(100);
}

// действия при вращении энкодера
ISR (INT1_vect){
if (PIND & (1 << ENCB)){
    if (PIND & (1 << SW)) {if (mode == 1){TEH_sp -= 5;} else {TEH_op -= 5;}} else {speedFan -= 10;}  
 } else {
    if (PIND & (1 << SW)) {if (mode == 1){TEH_sp += 5;} else {TEH_op += 5;}} else {speedFan += 10;} 
 }
}

void setup(void) {
// таймер-счетчик времени
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A = 0xBB; // 187 или 188 (0xBC)
    TIMSK0 = (1 << OCIE0A);  

// таймер - ШИМ (fast PWM 8 bit)
    TCCR1A = (1 << COM1B1) | (0 << COM1B0) | (0 << WGM01) | (1 << WGM00);
    TCCR1B = (0 << WGM12) | (1 << CS12);
    OCR1BH = 0x00;
    

// порты ввода-вывода
    DDRB = (1 << FAN) | (1 << TEH); 
    DDRD = 0x00;
    
    EICRA = (1 << ISC11) | (1 << ISC01);
    EIMSK = (1 << INT1) | (1 << INT0);


// общие
    sei();
    
    twi_init(); 
    spi_init();   
}

/* подготовка строки для вывода на дисплей */
void PrepareString(char* _str, int _data){
  //char str[3];
  if ((_data >= 0) && (_data <= 999)){
    if (_data < 10) {sprintf(_str,"%s %u", " ", _data); }
    else if (_data < 100) {sprintf(_str,"%s %u", "",_data); }
    else  {sprintf(_str,"%u", _data); }
  }
}
/* --------------------------------------- */

/* низкочастотный ШИМ для ТЕНа  */
void SetOutTEH(int _output){
    if (_output <= 0) {
        PORTB &= ~(1 << TEH);
    } 
    else if (_output >= 100) {
        PORTB |= (1 << TEH);
    } 
    else {            
      if (millis < (uint32_t)(TEH_PWM_lastTime + (int)(TEH_PWM_PERIOD*(double)_output/100.0))) {
        PORTB |= (1 << TEH);
      }
      else {
        PORTB &= ~(1 << TEH);
      }
    }
    if (millis > (TEH_PWM_lastTime + TEH_PWM_PERIOD)){
        TEH_PWM_lastTime = millis;
    }
}
/* ----------------------------- */

/* установка скорости вентилятора */
void SetSpeedFan(int _speedFan) {
    if (_speedFan > 100) {speedFan = 100;}
    else if (_speedFan < 0) {speedFan = 0;}
    else {OCR1BL = speedFan*2.55;}
}
/* ------------------------------ */

/* чтение температуры */
void ReadTemperature(void) {
    uint16_t _temperature = spi_read_word();
    if (_temperature == 0xFF) {
      //термопара наисправна
      //и с этим надо что-то делать
    }
    else {
      temperature = (temperature * (MEASUREMENT_FILTER_CONST - 1) +(float)_temperature*0.25)/MEASUREMENT_FILTER_CONST;
    }
} 
/* ------------------ */

/* обработчик нажатия кнопки энкодера */
void EncoderButtonPress(void){
  if ((swPressed == 0) && (~PIND & (1 << SW))){
    swPressed = 1;
    lastTimeSWPressed = millis;
  }
  if ((swPressed == 1) && ((millis - lastTimeSWPressed) > SW_PRESS_TIME)){
    if (~PIND & (1 << SW)) {
      mode = ~mode & 0x01;
      displayUpdateFlag = 1;
    }
    swPressed = 0;    
  }   
  if (PIND & (1 << SW)){
    swPressed = 0;
  }
}    
/* ---------------------------------- */

/* процедура вывода на дисплей */
void DisplayPrint(void) {
  if (displayUpdateFlag == 1){        //перерисовка дисплея
    displayUpdateFlag = 0;
    ssd1306_clear();
    if (mode == 1){
      ssd1306_img32x32(ssd1306xled_images, 0, 0, 0);
      ssd1306_img32x32(ssd1306xled_images, 96, 0, 2);
    }
    else {
      ssd1306_img32x32(ssd1306xled_images, 0, 0, 4);      
      if (showOPFlag == 0) {        
        ssd1306_img32x32(ssd1306xled_images, 96, 0, 2);
      }
      else {
        ssd1306_img32x32(ssd1306xled_images, 96, 0, 3);
      }
    }
    ssd1306_img32x32(ssd1306xled_images, 0, 4, 1);    
    ssd1306_img32x32(ssd1306xled_images, 96, 4, 3);    
    } 


  char str[3];      //вывод параметров регулятора
  if (mode == 1){   //<<---сюда вставить переключение отображение задание/темература
    if ((spChanged == 1) && ((millis - lastTimeSPChange) < SHOW_SP_TIME)){
      PrepareString(str, TEH_sp);
      ssd1306tx_stringxy_16x32(ssd1306xled_font16x32data, 40, 0, str);
    }
    else {
      spChanged = 0;
      PrepareString(str, temperature);
      ssd1306tx_stringxy_16x32(ssd1306xled_font16x32data, 40, 0, str);
    }
  }
  else {
    if (showOPFlag == 1) {
      PrepareString(str, TEH_op);
      ssd1306tx_stringxy_16x32(ssd1306xled_font16x32data, 40, 0, str);
      ssd1306_img32x32(ssd1306xled_images, 96, 0, 3);
    }
    else {
      PrepareString(str, temperature);
      ssd1306tx_stringxy_16x32(ssd1306xled_font16x32data, 40, 0, str);
      ssd1306_img32x32(ssd1306xled_images, 96, 0, 2);
    }
  }

  //вывод скорости вентилятора
  PrepareString(str, speedFan);
  ssd1306tx_stringxy_16x32(ssd1306xled_font16x32data, 40, 4, str);
  
  countDisplayPrint++;  
  if (countDisplayPrint > DISPLAY_UPDATE_COUNT) {   //после заданного количества вызовов обновляем дисплей
    countDisplayPrint = 0;
    displayUpdateFlag = 1;
  }  
}
/* --------------------------- */

void main(void) {
    setup();

    _delay_ms(500);    

    ssd1306_init();
    ssd1306_clear();   
    ssd1306tx_init(ssd1306xled_font16x32data, ' ');
    
    
    while (1) {
        EncoderButtonPress();
        SetSpeedFan(speedFan);  

           
        //детектор изменения SP
        if (lastTEH_sp != TEH_sp) {
         lastTEH_sp = TEH_sp;
          spChanged = 1;
          lastTimeSPChange = millis;
        }


        //детектор изменения OP
        if ((TEH_op != lastTEH_op) && (mode == 0)) {
          lastTEH_op = TEH_op;
          showOPFlag = 1;
          lastTimeDisplayDataSwitch = millis;
        }
        //флаг изменения показаний в режиме МАН
        if (millis - lastTimeDisplayDataSwitch > DISPLAY_DATA_SWITCH_TIME) {
          showOPFlag = ((~showOPFlag) & 0x01);
          lastTimeDisplayDataSwitch = millis;
        }


        SetOutTEH(TEH_op);


        if (TEH_op < 0) {TEH_op = 0;} else if (TEH_op > 100) {TEH_op = 100;}   //такая себе проверочка       
        if (TEH_sp < 0) {TEH_sp = 0;} else if (TEH_sp > 300) {TEH_sp = 300;}        

        if (millis - lastTimeTempMeasure > MEASUREMENT_TIME){
            ReadTemperature();
            lastTimeTempMeasure = millis;
        }
        
        if (millis - lastTimeDisplayPrint > DISPLAY_PRINT_TIME) {                       
            DisplayPrint();            
            lastTimeDisplayPrint = millis;            
        }   
 

    }
}




