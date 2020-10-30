#ifndef LEUART_HG
#define LEUART_HG
//***********************************************************************************
// Include files
//***********************************************************************************

#include "em_leuart.h"
#include "sleep_routines.h"


//***********************************************************************************
// defined files
//***********************************************************************************

#define LEUART_TX_EM		EM3
#define LEUART_RX_EM		EM3
#define TEST_BITS           0xFF

/***************************************************************************//**
 * @addtogroup leuart
 * @{
 ******************************************************************************/

typedef enum{
	LEUART_Start,
	Transmit_data,
	Wait_finish,
	Transmission_end

}eLEUART_States;

typedef struct {
	uint32_t						baudrate;
	LEUART_Databits_TypeDef			databits;
	LEUART_Enable_TypeDef			enable;
	LEUART_Parity_TypeDef 			parity;
	uint32_t						ref_freq;
	LEUART_Stopbits_TypeDef			stopbits;
	bool							rxblocken;
	bool							sfubrx;
	bool							startframe_en;
	char							startframe;
	bool							sigframe_en;
	char							sigframe;
	uint32_t						rx_loc;
	uint32_t						rx_pin_en;
	uint32_t						tx_loc;
	uint32_t						tx_pin_en;
	bool							rx_en;
	bool							tx_en;
	uint32_t						rx_done_evt;
	uint32_t						tx_done_evt;
} LEUART_OPEN_STRUCT;

typedef struct {
	eLEUART_States		state;
	LEUART_TypeDef 		*leuart;
	uint32_t 			message_length;
	char				message[80];
	int					index;

}LEUART_PAYLOAD;




/** @} (end addtogroup leuart) */

//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************
void leuart_open(LEUART_TypeDef *leuart, LEUART_OPEN_STRUCT *leuart_settings);
void LEUART0_IRQHandler(void);
void leuart_start(LEUART_TypeDef *leuart, char *string, uint32_t string_len);
bool leuart_tx_busy(LEUART_TypeDef *leuart);

uint32_t leuart_status(LEUART_TypeDef *leuart);
void leuart_cmd_write(LEUART_TypeDef *leuart, uint32_t cmd_update);
void leuart_if_reset(LEUART_TypeDef *leuart);
void leuart_app_transmit_byte(LEUART_TypeDef *leuart, uint8_t data_out);
uint8_t leuart_app_receive_byte(LEUART_TypeDef *leuart);

#endif
