
#include "functions.h"
adc_cali_handle_t cali_handle= NULL;

/*############################################*/
/*############### GPIO-Setup #################*/
/*############################################*/
void configure_GPIO_dir(const char *TAG)
{
    /* reset every used GPIO-pin *
    gpio_reset_pin(CONFIG_HIN_U_GPIO);
    gpio_reset_pin(CONFIG_HIN_V_GPIO);
    gpio_reset_pin(CONFIG_HIN_W_GPIO);

    gpio_reset_pin(CONFIG_LIN_U_GPIO);
    gpio_reset_pin(CONFIG_LIN_V_GPIO);
    gpio_reset_pin(CONFIG_LIN_W_GPIO);
*/
    gpio_reset_pin(CONFIG_HALL_A_GPIO);
    gpio_reset_pin(CONFIG_HALL_B_GPIO);
    gpio_reset_pin(CONFIG_HALL_C_GPIO);

    //gpio_reset_pin(CONFIG_IN_ENC_A_GPIO); 
    gpio_reset_pin(CONFIG_IN_ENC_B_GPIO);
    gpio_reset_pin(CONFIG_IN_ENC_BUT_GPIO);
    //gpio_reset_pin(CONFIG_BUTTON_GPIO);

    
    gpio_reset_pin(CONFIG_EXT_ENC_LEFT_GPIO);
    gpio_reset_pin(CONFIG_EXT_ENC_RIGHT_GPIO);
    gpio_reset_pin(CONFIG_RFE_GPIO);

    /* Set the GPIO as a push/pull output
    gpio_set_direction(CONFIG_HIN_U_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_HIN_V_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_HIN_W_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(CONFIG_LIN_U_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_LIN_V_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_LIN_W_GPIO, GPIO_MODE_OUTPUT);
*/
    gpio_set_direction(CONFIG_HALL_A_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_HALL_B_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_HALL_C_GPIO, GPIO_MODE_INPUT);

    //gpio_set_direction(CONFIG_IN_ENC_A_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_IN_ENC_B_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_IN_ENC_BUT_GPIO, GPIO_MODE_INPUT);
    //gpio_set_direction(CONFIG_BUTTON_GPIO, GPIO_MODE_INPUT);

    
    gpio_set_direction(CONFIG_EXT_ENC_LEFT_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_EXT_ENC_RIGHT_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_RFE_GPIO, GPIO_MODE_INPUT);
    ESP_LOGI(TAG, "GPIO dirs configured for DIY power PCB");
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
/*################ PWM-Setup #################*/
/*############################################*/

void set_PWM_Timer()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode         = LEDC_HIGH_SPEED_MODE,
        .timer_num          = LEDC_TIMER_0,
        .duty_resolution    = LEDC_TIMER_10_BIT,
        .freq_hz            = CONFIG_FREQ_PWM_HIN,
        .clk_cfg            = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        printf("Fehler beim Konfigurieren des LEDC-Timers: %s\n", esp_err_to_name(err));
        return;
    }
   
}
void set_PWM()
{
        ledc_channel_config_t ledc_channel_HIN_U = 
    {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,    // Gleicher Modus wie beim Timer
        .channel        = LEDC_CHANNEL_0,          // Kanal 0 verwenden
        .timer_sel      = LEDC_TIMER_0,            // Timer 0 zuweisen
        .intr_type      = LEDC_INTR_DISABLE,       // Keine Interrupts
        .gpio_num       = CONFIG_HIN_U_GPIO,         
        .duty           = 0,                     //
        .hpoint         = 0                        // Start des PWM-Signals
    };
    ledc_channel_config(&ledc_channel_HIN_U);   // Kanal konfigurieren
        ledc_channel_config_t ledc_channel_HIN_V = 
    {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,    // Gleicher Modus wie beim Timer
        .channel        = LEDC_CHANNEL_1,          // Kanal 0 verwenden
        .timer_sel      = LEDC_TIMER_0,            // Timer 0 zuweisen
        .intr_type      = LEDC_INTR_DISABLE,       // Keine Interrupts
        .gpio_num       = CONFIG_HIN_V_GPIO,         
        .duty           = 0,                     // 
        .hpoint         = 0                        // Start des PWM-Signals
    };
    ledc_channel_config(&ledc_channel_HIN_V);   // Kanal konfigurieren
        ledc_channel_config_t ledc_channel_HIN_W = 
    {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,    // Gleicher Modus wie beim Timer
        .channel        = LEDC_CHANNEL_2,          // Kanal 0 verwenden
        .timer_sel      = LEDC_TIMER_0,            // Timer 0 zuweisen
        .intr_type      = LEDC_INTR_DISABLE,       // Keine Interrupts
        .gpio_num       = CONFIG_HIN_W_GPIO,         
        .duty           = 0,                     // 
        .hpoint         = 0                        // Start des PWM-Signals
    };
    ledc_channel_config(&ledc_channel_HIN_W);   // Kanal konfigurieren
}
void pwmStart(int PWM_CH, int Duty){
    ledc_set_duty(LEDC_HIGH_SPEED_MODE,PWM_CH, Duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE,PWM_CH);
}

void pwmStop(int PWM_CH){
    ledc_stop(LEDC_HIGH_SPEED_MODE, PWM_CH, 0);
}
void pwmStopAll(){
    ledc_stop(LEDC_HIGH_SPEED_MODE, HIN_U_CH, 0);
    ledc_stop(LEDC_HIGH_SPEED_MODE, HIN_V_CH, 0);
    ledc_stop(LEDC_HIGH_SPEED_MODE, HIN_W_CH, 0);
    gpio_set_level(CONFIG_LIN_U_GPIO, 0);      
    gpio_set_level(CONFIG_LIN_V_GPIO, 0);      
    gpio_set_level(CONFIG_LIN_W_GPIO, 0);      
}


void U_V_start(int duty)
{   
    //HIN_V und LIN_U abschalten
    pwmStop(HIN_V_CH);
    gpio_set_level(CONFIG_LIN_U_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_U_CH, duty);
    gpio_set_level(CONFIG_LIN_V_GPIO, 1);      
}
void V_U_start(int duty)
{
    //HIN_U und LIN_V abschalten
    pwmStop(HIN_U_CH);
    gpio_set_level(CONFIG_LIN_V_GPIO, 0);
    //HIN_V und LIN_U einschalten     
    pwmStart(HIN_V_CH, duty);
    gpio_set_level(CONFIG_LIN_U_GPIO, 1);  
}
void U_W_start(int duty)
{
    //HIN_W und LIN_U abschalten
    pwmStop(HIN_W_CH);
    gpio_set_level(CONFIG_LIN_U_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_W_CH, duty);
    gpio_set_level(CONFIG_LIN_V_GPIO, 1);      
}
void W_U_start(int duty)
{
    //HIN_U und LIN_W abschalten
    pwmStop(HIN_U_CH);
    gpio_set_level(CONFIG_LIN_W_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_W_CH, duty);
    gpio_set_level(CONFIG_LIN_U_GPIO, 1);     
}
void V_W_start(int duty)
{
    //HIN_U und LIN_W abschalten
    pwmStop(HIN_W_CH);
    gpio_set_level(CONFIG_LIN_V_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_V_CH, duty);
    gpio_set_level(CONFIG_LIN_W_GPIO, 1);     
}

void W_V_start(int duty)
{
    //HIN_U und LIN_W abschalten
    pwmStop(HIN_V_CH);
    gpio_set_level(CONFIG_LIN_W_GPIO, 0);
    //HIN_U und LIN_V einschalten     
    pwmStart(HIN_W_CH, duty);
    gpio_set_level(CONFIG_LIN_V_GPIO, 1);     
}

/*############################################*/
/*############### MCPWM-Setup ################*/
/*############################################*/
void conf_mcpwm_timers(){
    ESP_LOGI("MCPWM","started");
   mcpwm_timer_handle_t timer_U = NULL;
   mcpwm_timer_handle_t timer_V = NULL;
   mcpwm_timer_handle_t timer_W = NULL;
//creating timer configs and linking them with the timers
    mcpwm_timer_config_t timer_U_config = 
    {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 40000000, // 1MHz Auflösung
        .period_ticks = 2000,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_U_config, &timer_U));

    mcpwm_timer_config_t timer_V_config = 
    {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 40000000, // 1MHz Auflösung
        .period_ticks = 2000,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_V_config, &timer_V));

    mcpwm_timer_config_t timer_W_config = 
    {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 40000000, // 1MHz Auflösung
        .period_ticks = 2000,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_W_config, &timer_W));

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
        .count_value = 667, //120 degree delayed
    };
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(timer_V,&sync_phase_V_config));
//set Timer_W as an Slave of Timer_U with another phase 
    mcpwm_timer_sync_phase_config_t sync_phase_W_config = 
    {
        .sync_src = sync_signal,
        .count_value = 1333, //240 degree delayed
    };
    ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(timer_W,&sync_phase_W_config));    

//create Operators
    mcpwm_oper_handle_t operator_U = NULL;
    mcpwm_oper_handle_t operator_V = NULL;
    mcpwm_oper_handle_t operator_W = NULL; 
    //Operator for Timer_U
    mcpwm_operator_config_t operator_U_config = 
    {
        .group_id=0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_U_config,&operator_U));
    //Operator for Timer_V
    mcpwm_operator_config_t operator_V_config = 
    {
        .group_id=0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_V_config,&operator_V));
    //Operator for Timer_W
    mcpwm_operator_config_t operator_W_config = 
    {
        .group_id=0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_W_config,&operator_W));
    
    //connect PWM-Signals with Timers
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_U, timer_U));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_V, timer_V));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_W, timer_W));

    //create PWM-Signals
    mcpwm_cmpr_handle_t comperator_U = NULL;
    mcpwm_cmpr_handle_t comperator_V = NULL;
    mcpwm_cmpr_handle_t comperator_W = NULL;

    mcpwm_comparator_config_t comparator_U_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_U, &comparator_U_config,&comperator_U));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_U, 1000));//=50% Duty cycle

    mcpwm_comparator_config_t comparator_V_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_V, &comparator_V_config,&comperator_V));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_V, 1000));//=50% Duty cycle

    mcpwm_comparator_config_t comparator_W_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_W, &comparator_W_config,&comperator_W));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comperator_W, 1000));//=50% Duty cycle
//create generators for every pin
    mcpwm_gen_handle_t generator_U_HIN = NULL;
    mcpwm_gen_handle_t generator_V_HIN = NULL;
    mcpwm_gen_handle_t generator_W_HIN = NULL;
    mcpwm_gen_handle_t generator_U_LIN = NULL;
    mcpwm_gen_handle_t generator_V_LIN = NULL;
    mcpwm_gen_handle_t generator_W_LIN = NULL;
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

    

   
    
    /*ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_LIN, generator_V_HIN,&deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_LIN, generator_W_HIN,&deadtime_config));*/
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_U_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_U, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_U_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_U, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_V_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_V, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_V_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_V, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_W_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_W, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_W_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_W, MCPWM_GEN_ACTION_LOW)));
    
    mcpwm_dead_time_config_t deadtime_config = {
        .posedge_delay_ticks = 20,
        .negedge_delay_ticks = 0,
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_U_HIN, generator_U_HIN,&deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_HIN, generator_V_HIN,&deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_HIN, generator_W_HIN,&deadtime_config));
    deadtime_config.posedge_delay_ticks = 0;
    deadtime_config.negedge_delay_ticks = 20;
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_U_HIN, generator_U_LIN, &deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_HIN, generator_V_LIN, &deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_HIN, generator_W_LIN, &deadtime_config));
    

    }
/*############################################*/
/*################## MISC ####################*/
/*############################################*/
//Ausgelagert in Preprocessing python program, generate_pins_header.py
void parse_3pins(const char *TAG, const char *pin_string, int *pins) {
    int pin_count = 0;  // Jetzt ein Integer, keine Null-Pointer-Dereferenzierung
    char *token;
    char *pin_list = strdup(pin_string);  // Kopie der String-Option

    token = strtok(pin_list, ",");
    while (token != NULL && pin_count < 3) { // maximal 3 Pins
        pins[pin_count] = atoi(token);   // Umwandlung in Integer
        pin_count++;
        token = strtok(NULL, ",");
    }
    free(pin_list);  // Speicher freigeben
}