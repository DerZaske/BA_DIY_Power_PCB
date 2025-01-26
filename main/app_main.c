/* 
This is the first try of a Test-Software for the DIY Power PCB by Fabian Zaske
*/
#include <stdio.h>
#include <stdlib.h>
#include "functions.h"
#include "GPIO.h"
#include "mcpwm.h"
#include "ADC.h"
#include "string.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "string.h"
#include "parsed_pins.h"
#include "sdkconfig.h"
#include <stdbool.h>
#include <stdint.h>


void app_main(void)
{
    uint32_t Torque = 0;
    uint32_t Voltage_IN = 0;
    int32_t Current_U = 0;
    int32_t Current_V = 0;
    int32_t Current_W = 0;
    int32_t Current_bridge =0;
    int16_t enc_counter = 0;
    uint16_t menu_counter = 0;
    bool Hall_A_On = false;
    bool Hall_B_On = false;
    bool Hall_C_On = false;
    bool RFE_Pulled = false;
    int direction = 0;
    float Speed_indx = 0.0;
    float Speed_AB = 0.0;
    float duty = 0.0;
    char display_message[50]; // Puffer für die Nachricht
    bool enc_but_state = false;
    bool in_menu = false;
    configure_GPIO_dir();
   
    SSD1306_t *dev_pt = configure_OLED();
    gpio_set_level(CONFIG_HIN_U_GPIO,1);
    mcpwm_init();
    int i =0;
    set_enc_in_counter(menu_counter);
        
    
    //gpio_set_level(CONFIG_HIN_V_GPIO, 1);
    while (1) {
        //ssd1306_clear_screen(dev_pt, false);
        /* Die Anzeige der OLED mit der richtigen Nachricht
        Torque = get_torque(adc1_handle);
        Voltage_IN = get_voltage_in(adc1_handle);
        Current_U = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_U_ADC);
        Current_V = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_V_ADC);
        Current_W = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_W_ADC);
        */
       /* Hall_A_On = get_Hall(CONFIG_HALL_A_GPIO);
        Hall_B_On = get_Hall(CONFIG_HALL_B_GPIO);
        Hall_C_On = get_Hall(CONFIG_HALL_C_GPIO);
        */
        
        //Speed_indx = get_speed_index();
        //Speed_AB = get_speed_AB();
        //direction = get_direction();

    
        RFE_Pulled = !(gpio_get_level(CONFIG_RFE_GPIO));

        
        
        menu_counter = get_enc_in_counter();
        if (menu_counter >= 4){
        menu_counter=0;
        set_enc_in_counter(0);
        }
        enc_but_state = get_enc_in_but();
        if (enc_but_state){
            in_menu ^= 1;
        }
        //Current_bridge = get_current_bridge(adc1_handle, CONFIG_I_SENSE_ADC);
        //gpio_set_level(CONFIG_LIN_U_GPIO,1);
             
        
        snprintf(display_message, sizeof(display_message), "PWM-Param.");
        ssd1306_display_text(dev_pt, 1, display_message, strlen(display_message), false);

        snprintf(display_message, sizeof(display_message), "PWMFreq.: %ik   ", (CONFIG_FREQ_PWM/1000));
        ssd1306_display_text(dev_pt, 3, display_message, 14, !(menu_counter));

        snprintf(display_message, sizeof(display_message), "Duty: %i     ", CONFIG_DUTY_PWM);
        ssd1306_display_text(dev_pt, 4, display_message, 14, !(menu_counter-1));

        snprintf(display_message, sizeof(display_message), "DeadTime: %i  ", CONFIG_DEAD_TIME_PWM);
        ssd1306_display_text(dev_pt, 5, display_message, 14, !(menu_counter-2));

        if (RFE_Pulled){
        snprintf(display_message, sizeof(display_message), "RFE pulled   ");
        }
        else{
        snprintf(display_message, sizeof(display_message), "RFE not pulled");
        }
        ssd1306_display_text(dev_pt, 7, display_message, 14, !(menu_counter-3));

       /* snprintf(display_message, sizeof(display_message), "Torque: %lu", Torque);
        ssd1306_display_text(dev_pt, 2, display_message, 11, false);    

        snprintf(display_message, sizeof(display_message), "Voltage: %lu",Voltage_IN);
        ssd1306_display_text(dev_pt, 3, display_message, strlen(display_message), false);

        snprintf(display_message, sizeof(display_message), "Current: %ldmA",Current_bridge);
        ssd1306_display_text(dev_pt, 4, display_message, strlen(display_message), false);

        snprintf(display_message, sizeof(display_message), "U: %ldmA",Current_U);
        ssd1306_display_text(dev_pt, 5, display_message, strlen(display_message), false);

        snprintf(display_message, sizeof(display_message), "V: %ldmA",Current_V);
        ssd1306_display_text(dev_pt, 6, display_message, strlen(display_message), false);

        snprintf(display_message, sizeof(display_message), "W: %ldmA",Current_W);
        ssd1306_display_text(dev_pt, 7, display_message, strlen(display_message), false);
        *///gpio_set_level(CONFIG_RFE_GPIO,0);
       
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Verzögerung für die Task-Schleife
        //i++;
    }
}
