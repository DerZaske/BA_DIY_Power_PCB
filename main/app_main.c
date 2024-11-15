/* 
This is the first try of a Test-Software for the DIY Power PCB by Fabian Zaske
*/
#include "functions.h"

const char *TAG = "Main_test";

void app_main(void)
{
    uint32_t Torque = 0;
    uint32_t Voltage_IN = 0;
    int32_t Current_U = 0;
    int32_t Current_V = 0;
    int32_t Current_W = 0;
    char display_message[50]; // Puffer für die Nachricht
    ESP_LOGI(TAG, "Test");
    configure_GPIO_dir(TAG);
    adc_oneshot_unit_handle_t adc1_handle = configure_ADC1(TAG);    
    //SSD1306_t *dev_pt = configure_OLED(TAG);
    set_PWM_Timer();
    set_PWM();
    int i =0;
    
    //gpio_set_level(CONFIG_HIN_V_GPIO, 1);
    while (1) {
       // ssd1306_clear_screen(dev_pt, false);
        // Die Anzeige der OLED mit der richtigen Nachricht
        Torque = get_torque(adc1_handle);
        Voltage_IN = get_voltage_in(adc1_handle);
        Current_U = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_U_ADC);
        Current_V = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_U_ADC);
        Current_W = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_U_ADC);
        if (Voltage_IN >= 20000){
            //ssd1306_display_text(dev_pt, 1, "Bridge=ON", 10, false);
        switch (i)
        {
        case 0:
            V_U_start(); 
            break;
        case 1:
            V_W_start();
            break;
        case 2:
            U_W_start();
            break;
        case 3:
            U_V_start();
            break;
        case 4:
            W_V_start();
            break;
        case 5:
            W_U_start();
            i=0;
            break;

        
        default:
        pwmStopAll();
            break;
        }
      
        }else{
            //ssd1306_display_text(dev_pt, 1, "Bridge=OFF", 10, false);
            pwmStopAll();
        }
        
        snprintf(display_message, sizeof(display_message), "Torque: %lu", Torque);
        
       // ssd1306_display_text(dev_pt, 2, display_message, 11, false);    
        snprintf(display_message, sizeof(display_message), "Voltage: %lu",Voltage_IN);
       
       // ssd1306_display_text(dev_pt, 4, display_message, strlen(display_message), false);
        snprintf(display_message, sizeof(display_message), "U: %ldmA",Current_U);
       
       // ssd1306_display_text(dev_pt, 5, display_message, strlen(display_message), false);
        snprintf(display_message, sizeof(display_message), "V: %ldmA",Current_V);
     
       // ssd1306_display_text(dev_pt, 6, display_message, strlen(display_message), false);
        snprintf(display_message, sizeof(display_message), "W: %ldmA",Current_W);
       
       // ssd1306_display_text(dev_pt, 7, display_message, strlen(display_message), false);
        //gpio_set_level(CONFIG_RFE_GPIO,0);
       
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Verzögerung für die Task-Schleife
        i++;
    }
}
