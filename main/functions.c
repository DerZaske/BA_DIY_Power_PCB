
#include "functions.h"
#include <string.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "parsed_pins.h"
#include "sdkconfig.h"

/*############################################*/
/*############## Display-Setup ###############*/
/*############################################*/
SSD1306_t *configure_OLED()
{
    static SSD1306_t dev;
	//int center, top, bottom;
	//char lineChar[20];

    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ESP_LOGI("OLED", "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text_x3(&dev, 0, "Hello", 5, false);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ssd1306_clear_screen(&dev, false);
    return &dev;
}



/*############################################*/
/*############ Blockkommutierung #############*/
/*############################################*/
bool get_Hall(int HallSensorGPIO){
    char* TAG="";

    if(HallSensorGPIO == CONFIG_HALL_A_GPIO){
        TAG = "HALL_A";
    }else if(HallSensorGPIO == CONFIG_HALL_B_GPIO){
        TAG = "HALL_B";
    }
    else if(HallSensorGPIO == CONFIG_HALL_C_GPIO){
        TAG = "HALL_C";
    }else{
        TAG = "Undefinded";
    }

    bool level = gpio_get_level(HallSensorGPIO);

    if(level){
    ESP_LOGI(TAG, "HIGH");
    }else{
    ESP_LOGI(TAG,"LOW");
    }
    return level;
}
