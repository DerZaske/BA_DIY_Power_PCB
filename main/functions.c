
#include "functions.h"
adc_cali_handle_t cali_handle= NULL;
void configure_GPIO_dir(const char *TAG)
{
    /* reset every used GPIO-pin */
    gpio_reset_pin(CONFIG_HIN_U_GPIO);
    gpio_reset_pin(CONFIG_HIN_V_GPIO);
    gpio_reset_pin(CONFIG_HIN_W_GPIO);

    gpio_reset_pin(CONFIG_LIN_U_GPIO);
    gpio_reset_pin(CONFIG_LIN_V_GPIO);
    gpio_reset_pin(CONFIG_LIN_W_GPIO);

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

    /* Set the GPIO as a push/pull output */
    gpio_set_direction(CONFIG_HIN_U_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_HIN_V_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_HIN_W_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(CONFIG_LIN_U_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_LIN_V_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CONFIG_LIN_W_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(CONFIG_HALL_A_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_HALL_B_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_HALL_C_GPIO, GPIO_MODE_INPUT);

    //gpio_set_direction(CONFIG_IN_ENC_A_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_IN_ENC_B_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_IN_ENC_BUT_GPIO, GPIO_MODE_INPUT);
    //gpio_set_direction(CONFIG_BUTTON_GPIO, GPIO_MODE_INPUT);

    
    gpio_set_direction(CONFIG_EXT_ENC_LEFT_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_EXT_ENC_RIGHT_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_RFE_GPIO, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "GPIO dirs configured for DIY power PCB");
}
// Globale Variable für die Kalibrierung

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