#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "ssd1306.h"
#include "sdkconfig.h"
#include "parsed_pins.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "menu.h"
#include "esp_timer.h"
#include "mcpwm.h"


/*############################################*/
/*############## Display-Setup ###############*/
/*############################################*/
static SSD1306_t dev;
void configure_OLED()
{
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, -1);
    ESP_LOGI("OLED", "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text_x3(&dev, 0, "Power", 5, false);
    ssd1306_display_text_x3(&dev, 4, " PCB", 4, false);
   
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ssd1306_clear_screen(&dev, false);
}

/*############################################*/
/*############ Internal Encoder ##############*/
/*############################################*/
//Variablen
static volatile int enc_in_counter = 0;
static volatile int64_t last_interrupt_time = 0;
static volatile uint16_t last_interrupt_time_but = 0;
static volatile bool enc_in_button_flag = false;
static volatile uint8_t last_state = 0;

static void IRAM_ATTR enc_in_isr_handler(void *arg) {
    static uint64_t last_interrupt_time = 0;
    
    // Aktueller Zustand der Pins lesen
    uint8_t current_state = (gpio_get_level(CONFIG_IN_ENC_A_GPIO) << 1) | gpio_get_level(CONFIG_IN_ENC_B_GPIO);
    uint64_t interrupt_time = esp_timer_get_time();

    // Zustandswechsel-Logik (FSM) ohne starre Entprellzeit
    if (current_state != last_state) { 
        // Nur wenn der Wechsel signifikant verzögert ist (gute Flanke)
        if ((interrupt_time - last_interrupt_time) > CONFIG_IN_ENCODER_DEBOUNCE_TIME) {
            if ((last_state == 0b01 && current_state == 0b11) || 
                (last_state == 0b10 && current_state == 0b00)) {
                enc_in_counter++;  // Vorwärtsdrehen
            } else if ((last_state == 0b10 && current_state == 0b11) || 
                       (last_state == 0b01 && current_state == 0b00)) {
                enc_in_counter--;  // Rückwärtsdrehen
            }
            last_state = current_state;  // Zustand aktualisieren
            last_interrupt_time = interrupt_time;
        }
    }

}

static void IRAM_ATTR enc_in_but_isr_handler(void *arg) {
   uint64_t interrupt_time = esp_timer_get_time();
    
    // Entprellung: Verhindert die Erfassung von Störungen aufgrund von Prellung
    if (interrupt_time - last_interrupt_time_but > (CONFIG_IN_ENCODER_DEBOUNCE_TIME*1000)) {  //  Entprellungszeit
        last_interrupt_time_but = interrupt_time; // Entprellzeit zurücksetzen
        if (gpio_get_level(CONFIG_IN_ENC_BUT_GPIO)) {
            enc_in_button_flag = true;
        }

    }
}

void config_internal_Encoder(){
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_A_GPIO, enc_in_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_B_GPIO, enc_in_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_BUT_GPIO, enc_in_but_isr_handler, NULL));
}
/*############################################*/
/*############### Menu-Setup #################*/
/*############################################*/
typedef enum {
    MAIN_MENU,
    CONFIGURE_MENU,
    MORE_INFO_MENU
} MenuState;

typedef enum {
    MCPWM_MODE,
    BLDC_MODE,
    DC_BRUSHED_MODE,
    MODE_COUNT
}OperationMode;

const char *mode_names[] = {
        "MCPWM   ",
        "BLDC    ",
        "DC Brush"
    };

 const char *OutCombi_names[]= {
        "+U -V",
        "+U -W",
        "+V -W",
        "+V -U",
        "+W -U",
        "+W -V",
        " +U  ",
        " +V  ",
        " +W  "
    };

typedef enum {
    STATE_ACTIVE,
    STATE_DEAKTIVE,
    STATE_UV,
    STATE_OC
} BridgeState;

const char *state_names[] = {
        "Active     ",
        "Deaktive   ",
        "UV         ",
        "RFE set(OC)"
    };


//Globalevariablen
volatile MenuState current_menu = MAIN_MENU;
volatile OperationMode current_mode = MCPWM_MODE;
volatile BridgeState current_bridge_state = STATE_DEAKTIVE;
volatile OutCombis current_out_combi = OUT_U_V;

volatile bool ShouldState = false; //false==deaktive
int cursor_position = 0;
int max_cursor_pos = 0;
bool in_editing = false;
char display_message[20]; // Puffer für die Nachricht
bool flag;
static void check_button_pressed(){
    if (enc_in_button_flag){
    enc_in_button_flag=false;

        switch (current_menu){

            case MAIN_MENU:
                switch(cursor_position){
                case 0:
                    current_mode = (current_mode+1)% MODE_COUNT;
                    break;
                case 1:
                    ShouldState = !ShouldState;
                    if(ShouldState){
                    start_mcpwm_output();
                    }else{
                    stop_mcpwm_output();
                    }
                    break;
                case 2:
                    current_menu = CONFIGURE_MENU;
                    enc_in_counter=0;
                    break;
                case 3:
                    current_menu = MORE_INFO_MENU;
                    enc_in_counter=0;
                    break;
                case 4:
                    current_out_combi =(current_out_combi+1)%6;
                    stop_mcpwm_output();
                    configure_mcpwm_output(current_out_combi);
                    ShouldState = false;
                    break;
                default:
                    snprintf(display_message, sizeof(display_message), "ERROR");
                    ssd1306_display_text(&dev, 7, display_message, strlen(display_message), false);
                    break;
                }
                break;
            case CONFIGURE_MENU:
                switch(cursor_position){

                }

            default:
                ESP_LOGE("Menu","Not yet programmed");
                break;
        }
    }
}
static void getset_bridge_state(){

bool RFE_Pulled = !(gpio_get_level(CONFIG_RFE_GPIO));

    if (RFE_Pulled){
    current_bridge_state=STATE_OC;
    }else if(!ShouldState){
    current_bridge_state=STATE_DEAKTIVE;
    }else{
    current_bridge_state=STATE_ACTIVE;
    }
}

static void render_main_menu(){
    max_cursor_pos = 4;
    //Mode
    snprintf(display_message, sizeof(display_message), "Mode: %s", mode_names[current_mode]);
    ssd1306_display_text(&dev, 1, display_message, strlen(display_message), cursor_position == 0);

    //ShouldState Started oder Stopped
    snprintf(display_message, sizeof(display_message), "%s", ShouldState ? "Started" : "Stopped");
    ssd1306_display_text(&dev, 2, display_message, strlen(display_message), cursor_position == 1);

    //Configure_Menu
    snprintf(display_message, sizeof(display_message), "Configure ->");
    ssd1306_display_text(&dev, 3, display_message, strlen(display_message), cursor_position == 2);

    //More_Info_Menu
    snprintf(display_message, sizeof(display_message), "Sensor Info ->");
    ssd1306_display_text(&dev, 4, display_message, strlen(display_message), cursor_position == 3);

    //Output Selection
    snprintf(display_message, sizeof(display_message), "Out: %s",OutCombi_names[current_out_combi]);
    ssd1306_display_text(&dev, 5, display_message, strlen(display_message), cursor_position == 4);

    
    //State
    getset_bridge_state();
    snprintf(display_message, sizeof(display_message), "State: %s",state_names[current_bridge_state]);
    ssd1306_display_text(&dev, 6, display_message, strlen(display_message), true);

    
    /*snprintf(display_message, sizeof(display_message), "cursor: %i %s",cursor_position,enc_in_button_flag ?"in" : "out");
    ssd1306_display_text(&dev, 7, display_message, strlen(display_message), true);
    */}
    
static void render_config_menu(){
max_cursor_pos = 3;
switch(current_mode){
    case MCPWM_MODE:
    //Titel
    snprintf(display_message, sizeof(display_message), "Conf. MCPWM");
    ssd1306_display_text(&dev, 1, display_message, strlen(display_message), false);

    //Frequenz
    snprintf(display_message, sizeof(display_message), "PWMFreq.: %ik   ", (get_frequency()/1000));
    ssd1306_display_text(&dev, 2, display_message, strlen(display_message), cursor_position == 0);

    //Duty cycle
    snprintf(display_message, sizeof(display_message), "Duty: %.1f%%   ", get_duty());
    ssd1306_display_text(&dev, 3, display_message, strlen(display_message), cursor_position == 1);

    //Todzeit
    snprintf(display_message, sizeof(display_message), "DeadTime: %ins  ", CONFIG_DEAD_TIME_PWM);
    ssd1306_display_text(&dev, 4, display_message, strlen(display_message), cursor_position == 2);

    //Back
    snprintf(display_message, sizeof(display_message), "    Back    ");
    ssd1306_display_text(&dev, 5, display_message, strlen(display_message), cursor_position == 3);

    //State
    getset_bridge_state();
    snprintf(display_message, sizeof(display_message), "State: %s",state_names[current_bridge_state]);
    ssd1306_display_text(&dev, 6, display_message, strlen(display_message), true);

    break;
    default:

}
}

static void render_info_menu(){
max_cursor_pos = 8;
}


void menu_loop(){
    if (enc_in_counter<0){
        enc_in_counter = max_cursor_pos;
    }
    if (enc_in_counter> max_cursor_pos){
        enc_in_counter =0;
    }
    cursor_position = enc_in_counter;
    switch (current_menu)
    {
    case MAIN_MENU:
        render_main_menu();
        break;
    case CONFIGURE_MENU:
        render_config_menu();
        break;
    case MORE_INFO_MENU:
        render_info_menu();
    default:
        break;
    }
   
   check_button_pressed();
}