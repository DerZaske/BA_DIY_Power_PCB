// functions.h
#ifndef FUNCTIONS_H
#define FUNCTIONS_H


#include "ssd1306.h"
#include <stdbool.h>


bool get_Hall(int HallSensorGPIO);
SSD1306_t *configure_OLED_old();

#endif