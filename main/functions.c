
#include "functions.h"
uint64_t delta_index_time = 0;
uint64_t last_index_time = 0;
uint64_t delta_AB_time = 0;
volatile int16_t enc_in_counter = 0;
volatile unsigned long last_interrupt_time_a = 0; // Entprellungs-Timer
volatile unsigned long last_interrupt_time_b = 0; // Entprellungs-Timer
volatile uint16_t last_interrupt_time_but = 0;
volatile bool enc_in_button_state = false;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

uint64_t last_AB_time = 0;    // Definition der Variablen

adc_cali_handle_t cali_handle = NULL;

/*############################################*/
/*############### GPIO-Setup #################*/
/*############################################*/

void configure_GPIO_dir(const char *TAG)
{
    /* reset every used GPIO-pin *
    gpio_reset_pin(CONFIG_HIN_U_GPIO);
    gpio_reset_pin(CONFIG_HIN_V_GPIO);
    gpio_reset_pin(CONFIG_HIN_W_GPIO);
*/
  //  gpio_reset_pin(CONFIG_LIN_U_GPIO);
    gpio_reset_pin(CONFIG_LIN_V_GPIO);
   // gpio_reset_pin(CONFIG_LIN_W_GPIO);

    gpio_reset_pin(CONFIG_HALL_A_GPIO);
    gpio_reset_pin(CONFIG_HALL_B_GPIO);
    gpio_reset_pin(CONFIG_HALL_C_GPIO);

    gpio_reset_pin(CONFIG_IN_ENC_A_GPIO); 
    gpio_reset_pin(CONFIG_IN_ENC_B_GPIO);
    gpio_reset_pin(CONFIG_IN_ENC_BUT_GPIO);
    //gpio_reset_pin(CONFIG_BUTTON_GPIO);

    
    gpio_reset_pin(CONFIG_EXT_ENC_LEFT_GPIO);
    gpio_reset_pin(CONFIG_EXT_ENC_RIGHT_GPIO);
    
    gpio_reset_pin(CONFIG_RFE_GPIO);
    gpio_config_t io_conf_RFE = {};
    io_conf_RFE.intr_type = GPIO_INTR_DISABLE; // Keine Interrupts
    io_conf_RFE.mode = GPIO_MODE_INPUT;        // Als Eingang setzen
    io_conf_RFE.pin_bit_mask = (1ULL << CONFIG_RFE_GPIO); // Pin festlegen
    io_conf_RFE.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_RFE.pull_up_en = GPIO_PULLUP_DISABLE;     // Pull-up-Widerstand deaktivieren
    gpio_config(&io_conf_RFE);
    
    /* Set the GPIO as a push/pull output
    gpio_set_direction(CONFIG_HIN_U_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_HIN_V_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_HIN_W_GPIO, GPIO_MODE_OUTPUT);*/

//    gpio_set_direction(CONFIG_LIN_U_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_LIN_V_GPIO, GPIO_MODE_OUTPUT);
  //  gpio_set_direction(CONFIG_LIN_W_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(CONFIG_HALL_A_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_HALL_B_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_HALL_C_GPIO, GPIO_MODE_INPUT);

    gpio_set_direction(CONFIG_IN_ENC_A_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_IN_ENC_B_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(CONFIG_IN_ENC_B_GPIO, GPIO_PULLUP_ENABLE);
    gpio_set_direction(CONFIG_IN_ENC_BUT_GPIO, GPIO_MODE_INPUT);
    //gpio_set_direction(CONFIG_BUTTON_GPIO, GPIO_MODE_INPUT);

    
    gpio_set_direction(CONFIG_EXT_ENC_LEFT_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_EXT_ENC_RIGHT_GPIO, GPIO_MODE_INPUT);
  

    ESP_LOGI(TAG, "GPIO dirs configured for DIY power PCB");

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << CONFIG_EXT_ENC_INDX_GPIO)| (1ULL << CONFIG_HALL_A_GPIO)| (1ULL << CONFIG_IN_ENC_A_GPIO)| (1ULL << CONFIG_IN_ENC_B_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;  // Interrupt auf beiden Flanken
    gpio_config(&io_conf);

    

    gpio_config_t io_conf_negedge = {};
    io_conf_negedge.pin_bit_mask = (1ULL << CONFIG_IN_ENC_BUT_GPIO);
    io_conf_negedge.mode = GPIO_MODE_INPUT;
    io_conf_negedge.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_negedge.intr_type = GPIO_INTR_POSEDGE; // Interrupt nur auf positive Flanken
    gpio_config(&io_conf_negedge);

    gpio_install_isr_service(0);
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_EXT_ENC_INDX_GPIO, index_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_HALL_A_GPIO, enc_ab_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_A_GPIO, enc_in_a_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_B_GPIO, enc_in_b_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_IN_ENC_BUT_GPIO, enc_in_but_isr_handler, NULL));
}
/*############################################*/
/*################ ADC-Setup #################*/
/*############################################*/
adc_oneshot_unit_handle_t configure_ADC1() 
{
    adc_oneshot_unit_handle_t adc1_handle;
    
    // ADC1 Initialisierung
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    // Kanal-Konfiguration
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_TORQUE_ADC, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_U_SENSE_ADC, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_I_SENSE_U_ADC, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_I_SENSE_V_ADC, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, CONFIG_I_SENSE_W_ADC, &config));

    // Kalibrierung initialisieren
    adc_cali_line_fitting_config_t cali_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGI("ADC", "ADC-Kalibrierung erfolgreich initialisiert");
    } else {
        ESP_LOGW("ADC", "ADC-Kalibrierung nicht möglich, Rohwerte werden verwendet");
        cali_handle = NULL;  // Keine Kalibrierung verfügbar
    }

    return adc1_handle;
}

uint32_t read_voltage(adc_oneshot_unit_handle_t adc1_handle, int channel) {
    int adc_raw = 0;
    int voltage_calibrated = 0;  // Verwende int für die Kalibrierungsfunktion
    uint32_t voltage = 0;         // Konvertiere später zu uint32_t

    // ADC-Rohwert lesen
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &adc_raw));

    // Kalibrierung anwenden, falls verfügbar
    if (cali_handle) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_calibrated));
        voltage = (uint32_t) voltage_calibrated;  // Konvertiere zu uint32_t
    } else {
        voltage = adc_raw;  // Fallback auf Rohwert
    }

    return voltage;
}
// Funktion zur Umrechnung in spezifische Spannung
uint32_t get_voltage_in(adc_oneshot_unit_handle_t adc1_handle)
{
    uint32_t adc_voltage = read_voltage(adc1_handle, CONFIG_U_SENSE_ADC);
    ESP_LOGI("ADC", "ADC%d:voltage:%ld", CONFIG_U_SENSE_ADC, adc_voltage);
    // Beispielhafte Umrechnung; Wert an eigene Anwendung anpassen
    uint32_t voltage_in = adc_voltage / 0.0909;
    return voltage_in;
}

int32_t get_current_ASC712(adc_oneshot_unit_handle_t adc1_handle, int ADC_pin)
{
    int32_t adc_voltage = read_voltage(adc1_handle,ADC_pin);
    int32_t current = (adc_voltage -2500)*5.405;
    ESP_LOGI("ADC", "ADC%d:voltage:%ldcurrent%ld", ADC_pin, adc_voltage, current);
    return current;
}

uint32_t get_torque(adc_oneshot_unit_handle_t adc1_handle)
{
    uint32_t adc_voltage =read_voltage(adc1_handle,CONFIG_TORQUE_ADC);
    uint32_t torque = adc_voltage/33;

    return torque;
}
int32_t get_current_bridge(adc_oneshot_unit_handle_t adc1_handle, int ADC_pin){
    int32_t adc_voltage = read_voltage(adc1_handle,ADC_pin);
     ESP_LOGI("CurrentBridge", "ADC:%ld",adc_voltage);
    int32_t current = ((adc_voltage- 142)/6.77)/0.007;
    return current;
}

/*############################################*/
/*############## Display-Setup ###############*/
/*############################################*/
SSD1306_t *configure_OLED(const char *TAG)
{
    static SSD1306_t dev;
	//int center, top, bottom;
	//char lineChar[20];

    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ESP_LOGI(TAG, "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text_x3(&dev, 0, "Hello", 5, false);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ssd1306_clear_screen(&dev, false);
    return &dev;
}


/*############################################*/
/*############### MCPWM-Setup ################*/
/*############################################*/
void conf_mcpwm_timers(){
    ESP_LOGI("MCPWM","started");
   mcpwm_timer_handle_t timer_U = NULL;
   mcpwm_timer_handle_t timer_V = NULL;
   mcpwm_timer_handle_t timer_W = NULL;
   uint32_t periode_ticks = CONFIG_TIMER_BASE_FREQ/CONFIG_FREQ_PWM;
   double tick_period_ns = 1e9 / CONFIG_TIMER_BASE_FREQ; // Zeit pro Tick in ns
   uint32_t dead_time_ticks = (uint32_t)round(CONFIG_DEAD_TIME_PWM / tick_period_ns);

//creating timer configs and linking them with the timers
    mcpwm_timer_config_t timer_config = 
    {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 40000000, //40MHz
        .period_ticks = periode_ticks,      //40MHz/2KHz = 20KHz
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer_U));
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer_V));
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer_W));

    ESP_ERROR_CHECK(mcpwm_timer_enable(timer_U));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer_U,MCPWM_TIMER_START_NO_STOP));
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer_V));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer_V,MCPWM_TIMER_START_NO_STOP));
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer_W));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer_W,MCPWM_TIMER_START_NO_STOP));


//set Timer_U as an sync_signal
    mcpwm_sync_handle_t sync_signal = NULL;
    mcpwm_timer_sync_src_config_t sync_src_config = 
    {
        .flags.propagate_input_sync = false,
        .timer_event = MCPWM_TIMER_EVENT_EMPTY,

    };
    ESP_ERROR_CHECK(mcpwm_new_timer_sync_src(timer_U,&sync_src_config, &sync_signal));
//set Timer_V as an Slave of Timer_U with another phase 
    mcpwm_timer_sync_phase_config_t sync_phase_V_config = 
    {
        .sync_src = sync_signal,
        .count_value = periode_ticks/3, //120 degree delayed
    };
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(timer_V,&sync_phase_V_config));
//set Timer_W as an Slave of Timer_U with another phase 
    mcpwm_timer_sync_phase_config_t sync_phase_W_config = 
    {
        .sync_src = sync_signal,
        .count_value = periode_ticks*2/3, //240 degree delayed
    };
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(timer_W,&sync_phase_W_config));    

//create Operators
    mcpwm_oper_handle_t operator_U = NULL;
    mcpwm_oper_handle_t operator_V = NULL;
    mcpwm_oper_handle_t operator_W = NULL; 

    //Operator for Timer_U
    mcpwm_operator_config_t operator_config = 
    {
        .group_id=0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config,&operator_U));
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config,&operator_V));
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config,&operator_W));
    
    //connect PWM-Signals with Timers
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_U, timer_U));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_V, timer_V));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_W, timer_W));

    //create PWM-Signals
    mcpwm_cmpr_handle_t comperator_U = NULL;
    mcpwm_cmpr_handle_t comperator_V = NULL;
    mcpwm_cmpr_handle_t comperator_W = NULL;

    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_U, &comparator_config,&comperator_U));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_U, periode_ticks*CONFIG_DUTY_PWM/100));//Duty_cycle from Config

    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_V, &comparator_config,&comperator_V));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_V, periode_ticks*CONFIG_DUTY_PWM/100));

    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_W, &comparator_config,&comperator_W));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_W,periode_ticks*CONFIG_DUTY_PWM/100));

//create generators for every pin
    mcpwm_gen_handle_t generator_U_HIN = NULL;
    mcpwm_gen_handle_t generator_V_HIN = NULL;
    mcpwm_gen_handle_t generator_W_HIN = NULL;
    mcpwm_gen_handle_t generator_U_LIN = NULL;
    mcpwm_gen_handle_t generator_V_LIN = NULL;
    mcpwm_gen_handle_t generator_W_LIN = NULL;
    mcpwm_gen_handle_t *mcpwm_gens[] ={&generator_U_HIN,&generator_U_LIN,&generator_V_HIN,&generator_V_LIN,&generator_W_HIN,&generator_W_LIN};
//HIN Pins
    //HIN_U
    mcpwm_generator_config_t generator_U_HIN_config ={
        .gen_gpio_num = CONFIG_HIN_U_GPIO,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_U, &generator_U_HIN_config, &generator_U_HIN));

    //HIN_V
    mcpwm_generator_config_t generator_V_HIN_config ={
        .gen_gpio_num = CONFIG_HIN_V_GPIO,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_V, &generator_V_HIN_config, &generator_V_HIN));

    //HIN_W
    mcpwm_generator_config_t generator_W_HIN_config ={
        .gen_gpio_num = CONFIG_HIN_W_GPIO,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_W, &generator_W_HIN_config, &generator_W_HIN));

    //LIN_U
    mcpwm_generator_config_t generator_U_LIN_config ={
        .gen_gpio_num = CONFIG_LIN_U_GPIO,
        .flags.invert_pwm = 1,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_U, &generator_U_LIN_config, &generator_U_LIN));

    //LIN_V
    mcpwm_generator_config_t generator_V_LIN_config ={
        .gen_gpio_num = CONFIG_LIN_V_GPIO,
        .flags.invert_pwm = 1,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_V, &generator_V_LIN_config, &generator_V_LIN));

     //LIN_W
    mcpwm_generator_config_t generator_W_LIN_config ={
        .gen_gpio_num = CONFIG_LIN_W_GPIO,
        .flags.invert_pwm = 1,
        .flags.pull_down = 1,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_W, &generator_W_LIN_config, &generator_W_LIN));

    //set generator action on timer event    
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_U_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_U, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_U_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_U, MCPWM_GEN_ACTION_LOW)));
   /* ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_V_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_V, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_V_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_V, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_W_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_W, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_W_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_W, MCPWM_GEN_ACTION_LOW)));
    */
    //set Dead times
    mcpwm_dead_time_config_t deadtime_config = {
        .posedge_delay_ticks = dead_time_ticks,
        .negedge_delay_ticks = 0,
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_U_HIN, generator_U_HIN,&deadtime_config));
   // ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_HIN, generator_V_HIN,&deadtime_config));
   // ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_HIN, generator_W_HIN,&deadtime_config));
    deadtime_config.posedge_delay_ticks = 0;
    deadtime_config.negedge_delay_ticks = dead_time_ticks;
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_U_HIN, generator_U_LIN, &deadtime_config));
   //ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_HIN, generator_V_LIN, &deadtime_config));
   //ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_HIN, generator_W_LIN, &deadtime_config));
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

/*############################################*/
/*############### Ext Encoder ################*/
/*############################################*/
void IRAM_ATTR index_isr_handler(void *arg){
    uint64_t current_time = esp_timer_get_time();

    if (last_index_time != 0){
        delta_index_time = current_time - last_index_time;
    }
    last_index_time = current_time;
}
void IRAM_ATTR enc_ab_isr_handler(void *arg){
    uint64_t current_time = esp_timer_get_time();

    if (last_AB_time != 0){
        delta_AB_time = current_time - last_AB_time;
    }
    last_AB_time = current_time;
}
int get_direction(){//-1=Error,0=right,1=left
    bool right = gpio_get_level(CONFIG_EXT_ENC_RIGHT_GPIO);
    bool left = gpio_get_level(CONFIG_EXT_ENC_LEFT_GPIO);
    int direction;
    if (left && right){
        direction= -1;
        ESP_LOGI("Encoder","Direction: Error");
    }else if(right){
        direction = 0;
        ESP_LOGI("Encoder","Direction: Right");
    }else{
        direction = 1;
           ESP_LOGI("Encoder","Direction: Left");
    }
    return direction;
}
float get_speed_index(){
    uint64_t local_delta_time = delta_index_time;
    float speed_rpm = 0;
    if (local_delta_time>0){
        speed_rpm = (60.0*1000000.0/local_delta_time);
        ESP_LOGI("Encoder", "Geschwindigkeit_Indx: %.2f RPM", speed_rpm);
    }
return speed_rpm;
}
float get_speed_AB(){
    uint64_t local_delta_time = delta_AB_time;
    float speed_rpm = 0;
    if (local_delta_time>0){
        speed_rpm = (60.0*1000000.0/local_delta_time)/1000;
        ESP_LOGI("Encoder", "Geschwindigkeit_AB: %.2f RPM", speed_rpm);
    }
return speed_rpm;
}
/*############################################*/
/*############ Internal Encoder ##############*/
/*############################################*/
void IRAM_ATTR enc_in_a_isr_handler(void *arg) {
    uint64_t interrupt_time = esp_timer_get_time();
    
    // Entprellung: Verhindert die Erfassung von Störungen aufgrund von Prellung
    if (interrupt_time - last_interrupt_time_a > (CONFIG_IN_ENCODER_DEBOUNCE_TIME*1000)) {  //  Entprellungszeit
        last_interrupt_time_a = interrupt_time; // Entprellzeit zurücksetzen
        // Bestimmen der Richtung anhand des Zustands von Pin A und B
        if (gpio_get_level(CONFIG_IN_ENC_A_GPIO)==gpio_get_level(CONFIG_IN_ENC_B_GPIO)) {
            enc_in_counter++; // Drehung nach links
        }
        
    }
}

void IRAM_ATTR enc_in_b_isr_handler(void *arg) {
   uint64_t interrupt_time = esp_timer_get_time();
    
    // Entprellung: Verhindert die Erfassung von Störungen aufgrund von Prellung
    if (interrupt_time - last_interrupt_time_b > (CONFIG_IN_ENCODER_DEBOUNCE_TIME*1000)) {  //  Entprellungszeit
        last_interrupt_time_b = interrupt_time; // Entprellzeit zurücksetzen
        // Bestimmen der Richtung anhand des Zustands von Pin A und B
        if (gpio_get_level(CONFIG_IN_ENC_A_GPIO)==gpio_get_level(CONFIG_IN_ENC_B_GPIO)) {
            enc_in_counter--;
        }

    }
}

void IRAM_ATTR enc_in_but_isr_handler(void *arg) {
   uint64_t interrupt_time = esp_timer_get_time();
    
    // Entprellung: Verhindert die Erfassung von Störungen aufgrund von Prellung
    if (interrupt_time - last_interrupt_time_but > (CONFIG_IN_ENCODER_DEBOUNCE_TIME*1000)) {  //  Entprellungszeit
        last_interrupt_time_but = interrupt_time; // Entprellzeit zurücksetzen
        // Bestimmen der Richtung anhand des Zustands von Pin A und B
        if (gpio_get_level(CONFIG_IN_ENC_A_GPIO)) {
            enc_in_button_state = true;
        }

    }
}
int16_t get_enc_in_counter(){
ESP_LOGI("Encoder_Int","Counter:%i",enc_in_counter);
return enc_in_counter;
}
void set_enc_in_counter(int16_t inital_value){
    enc_in_counter = inital_value;
}

bool get_enc_in_but(){
    if (enc_in_button_state){
        enc_in_button_state = false;
        return true;
    }
    else{
        return false;
    }
}