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
void set_mcpwm_output(Phase highside, Phase lowside, float Duty);
void set_mcpwm_duty(float Duty);
void get_comps(mcpwm_cmpr_handle_t comps[3]);
#endif