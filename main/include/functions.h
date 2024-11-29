// functions.h
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/ledc.h"
#include <string.h>
#include <stdlib.h>
#include "parsed_pins.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "ssd1306.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/mcpwm_types.h"
#include "driver/mcpwm_prelude.h"
#include "esp_timer.h"


#define HIN_U_CH 0
#define HIN_V_CH 1
#define HIN_W_CH 2

#ifndef MY_COMPONENT_H
#define MY_COMPONENT_H
extern adc_cali_handle_t cali_handle;
extern uint64_t delta_index_time;
extern uint64_t last_index_time;
extern uint64_t delta_AB_time;
extern uint64_t last_AB_time;

// Deklaration der Funktion, die in my_component.c implementiert ist
void configure_GPIO_dir(const char *TAG);
adc_oneshot_unit_handle_t configure_ADC1();
uint32_t read_voltage(adc_oneshot_unit_handle_t adc1_handle, int channel);
uint32_t get_voltage_in(adc_oneshot_unit_handle_t adc1_handle);
uint32_t get_torque(adc_oneshot_unit_handle_t adc1_handle);
int32_t get_current_ASC712(adc_oneshot_unit_handle_t adc1_handle, int ADC_pin);
int32_t get_current_bridge(adc_oneshot_unit_handle_t adc1_handle, int ADC_pin);
void set_PWM_Timer();
void set_PWM();
void pwmStart(int PWM_CH,int Duty);
void pwmStop(int PWM_CH);
void pwmStopAll();
void U_V_start(int duty);
void V_U_start(int duty);
void U_W_start(int duty);
void W_U_start(int duty);
void V_W_start(int duty);
void W_V_start(int duty);
bool get_Hall(int HallSensorGPIO);
int get_direction();
float get_speed_index();
float get_speed_AB();
void conf_mcpwm_timers();
void parse_3pins(const char *TAG, const char *pin_string, int *pins);
SSD1306_t *configure_OLED(const char *TAG);

#endif // MY_COMPONENT_H