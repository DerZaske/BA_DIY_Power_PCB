#ifndef MCPWM_H
#define MCPWM_H

typedef enum {
    PHASE_U,
    PHASE_V,
    PHASE_W
} Phase;

void mcpwm_init();
void set_mcpwm_output(Phase highside, Phase lowside, float Duty);
void set_mcpwm_duty(float Duty);

#endif