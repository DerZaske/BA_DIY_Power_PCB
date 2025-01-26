#ifndef GPIO_H
#define GPIO_H

#include "esp_adc/adc_oneshot.h"

extern adc_cali_handle_t cali_handle;

adc_oneshot_unit_handle_t configure_ADC1();
uint32_t get_voltage_in(adc_oneshot_unit_handle_t adc1_handle);
uint32_t get_torque(adc_oneshot_unit_handle_t adc1_handle);
int32_t get_current_ASC712(adc_oneshot_unit_handle_t adc1_handle, int ADC_pin);
int32_t get_current_bridge(adc_oneshot_unit_handle_t adc1_handle, int ADC_pin);
#endif