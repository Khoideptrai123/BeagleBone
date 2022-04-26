//Display 14 segments via I2C
#ifndef _SEGMENT_H_
#define _SEGMENT_H_

void setupI2C();
void delay();
void turnOffBothDigits();
void turnOnLeftDigit();
void turnOnRightDigit();
void displaySingleDigit(int num);
void displayNumberViaI2C(int num);
void i2c_startDisplay();
void* i2c_display_thread();
void i2c_stopDisplay();


#endif 