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
    int32_t Current_bridge =0;
    bool Hall_A_On = false;
    bool Hall_B_On = false;
    bool Hall_C_On = false;
    int direction = 0;
    float Speed_indx = 0.0;
    float Speed_AB = 0.0;
    float duty = 0.0;
    char display_message[50]; // Puffer für die Nachricht
    ESP_LOGI(TAG, "Test");
    configure_GPIO_dir(TAG);
    adc_oneshot_unit_handle_t adc1_handle = configure_ADC1(TAG);    
    SSD1306_t *dev_pt = configure_OLED(TAG);
    conf_mcpwm_timers();
    int i =3;
    
    //gpio_set_level(CONFIG_HIN_V_GPIO, 1);
    while (1) {
        //ssd1306_clear_screen(dev_pt, false);
        // Die Anzeige der OLED mit der richtigen Nachricht
        Torque = get_torque(adc1_handle);
        Voltage_IN = get_voltage_in(adc1_handle);
        Current_U = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_U_ADC);
        Current_V = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_U_ADC);
        Current_W = get_current_ASC712(adc1_handle,CONFIG_I_SENSE_U_ADC);
        
       /* Hall_A_On = get_Hall(CONFIG_HALL_A_GPIO);
        Hall_B_On = get_Hall(CONFIG_HALL_B_GPIO);
        Hall_C_On = get_Hall(CONFIG_HALL_C_GPIO);
        */
        Speed_indx = get_speed_index();
        Speed_AB = get_speed_AB();
        direction = get_direction();

        Current_bridge = get_current_bridge(adc1_handle, CONFIG_I_SENSE_ADC);
        if (Voltage_IN >= 20000){
            ssd1306_display_text(dev_pt, 1, "Bridge=ON", 10, false);
        switch (i)
        {
        case 0:
            
            break;
        case 1:
           
            break;
        case 2:
           
            break;
        case 3:
            
            break;
        case 4:
           
            break;
        case 5:
           
            i=0;
            break;

        
        default:
       
            break;
        }
      
        }else{
            ssd1306_display_text(dev_pt, 1, "Bridge=OFF", 10, false);
            
        }
        
        snprintf(display_message, sizeof(display_message), "Torque: %lu", Torque);
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
        //gpio_set_level(CONFIG_RFE_GPIO,0);
       
        vTaskDelay(500 / portTICK_PERIOD_MS);  // Verzögerung für die Task-Schleife
        //i++;
    }
}
