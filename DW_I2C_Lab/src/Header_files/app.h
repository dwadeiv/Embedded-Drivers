//***********************************************************************************
// Include files
//***********************************************************************************
#include "em_cmu.h"
#include "em_prs.h"
#include "cmu.h"
#include "gpio.h"
#include "SI7021.h"

//***********************************************************************************
// defined files
//***********************************************************************************
#define		PWM_PER				3.1		// PWM period in seconds
#define		PWM_ACT_PER			0.10	// PWM active period in seconds
#define		LETIMER0_ROUTE_OUT0	LETIMER_ROUTELOC0_OUT0LOC_LOC28
#define		LETIMER0_OUT0_EN	false
#define		LETIMER0_ROUTE_OUT1	0
#define		LETIMER0_OUT1_EN	false
#define		LETIMER0_COMP0_EVT 	0x00000001	// 0b0001
#define		LETIMER0_COMP1_EVT 	0x00000002	// 0b0010
#define		LETIMER0_UF_EVT 	0x00000004	// 0b0100


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************
void app_peripheral_setup(void);
void app_letimer_pwm_open(float period, float act_period);
void scheduled_letimer0_uf_evt (void);
void scheduled_letimer0_comp0_evt (void);
void scheduled_letimer0_comp1_evt (void);
void scheduled_si7021_temp_done_evt(void);

