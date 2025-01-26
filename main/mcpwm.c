#include "mcpwm.h"
#include "hal/mcpwm_types.h"
#include "driver/mcpwm_prelude.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "math.h"
#include "parsed_pins.h"
#include "sdkconfig.h"

/*############################################*/
/*############### MCPWM-Setup ################*/
/*############################################*/
void mcpwm_init(){
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
   /* ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_U_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_U, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_U_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_U_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_U, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_V_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_V, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_V_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_V_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_V, MCPWM_GEN_ACTION_LOW)));
   */ ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_W_HIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_HIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_W, MCPWM_GEN_ACTION_LOW)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_W_LIN, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_W_LIN, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comperator_W, MCPWM_GEN_ACTION_LOW)));
    
    //set Dead times
    mcpwm_dead_time_config_t deadtime_config = {
        .posedge_delay_ticks = dead_time_ticks,
        .negedge_delay_ticks = 0,
    };

   // ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_U_HIN, generator_U_HIN,&deadtime_config));
   // ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_HIN, generator_V_HIN,&deadtime_config));
    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_HIN, generator_W_HIN,&deadtime_config));
    deadtime_config.posedge_delay_ticks = 0;
    deadtime_config.negedge_delay_ticks = dead_time_ticks;
    //ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_U_HIN, generator_U_LIN, &deadtime_config));
   //ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_V_HIN, generator_V_LIN, &deadtime_config));
   ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generator_W_HIN, generator_W_LIN, &deadtime_config));
    }
    