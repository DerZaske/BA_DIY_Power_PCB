#ifndef MCPWM_H
#define MCPWM_H
#include "hal/mcpwm_types.h"
#include "driver/mcpwm_prelude.h"
typedef enum {
    PHASE_U,
    PHASE_V,
    PHASE_W
} Phase;

void mcpwm_init();
esp_err_t set_mcpwm_output(Phase highside, Phase lowside);
esp_err_t set_mcpwm_duty(float duty);
esp_err_t set_mcpwm_frequenzy(uint16_t frequenzy);
void get_comps(mcpwm_cmpr_handle_t comps[3]);
float get_duty();
#endif