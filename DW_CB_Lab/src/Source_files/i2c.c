/**
 * @file i2c.c
 * @author David Wade
 * @date February 20th, 2020
 * @brief Contains all the functions for i2c peripheral
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "i2c.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// static functions/variables
//***********************************************************************************
static void i2c_ack();
static void i2c_nack();
static void i2c_rxdatav();
static void i2c_mstop();

static I2C_PAYLOAD payload;


//***********************************************************************************
// function prototypes
//***********************************************************************************


//***********************************************************************************
// functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * This function performs the necessary steps to get I2C ready for operation
 *
 * @details
 * 	It begins with enabling the necessary clocks for the i2C peripheral, initializing parameters for
 * 	communication, routing and enabling the scl and sda pins, then clearing and enabling the necessary interrupts
 * 	and calling the IRQHandler.
 *
 * @note
 * 	THis function will operate for either I2C0 or I2C1
 *
 * @param[in] I2C_TypeDef
 *
 * @param[in] I2C_OPEN_STRUCT
 *
 * @param[in] I2C_IO_STRUCT
 *
 ******************************************************************************/
void i2c_open(I2C_TypeDef *i2c, I2C_OPEN_STRUCT *i2c_setup, I2C_IO_STRUCT *i2c_info){
	I2C_Init_TypeDef i2c_values;
	if(i2c == I2C0){
		CMU_ClockEnable(cmuClock_I2C0 , true);
	}
	if(i2c == I2C1){
		CMU_ClockEnable(cmuClock_I2C1 , true);
	}
	if((i2c->IF & 0x01) == 0) {
		i2c->IFS = 0x01;
		EFM_ASSERT(i2c->IF & 0x01);
		i2c->IFC = 0x01;
	}
	else{
		i2c->IFC = 0x01;
		EFM_ASSERT(! (i2c->IF & 0x01));
	}
	// Initialize I2C
	i2c_values.clhr = i2c_setup->clhr;
	i2c_values.enable = i2c_setup->enable;
	i2c_values.freq = i2c_setup->freq;
	i2c_values.master = i2c_setup->master;
	i2c_values.refFreq = i2c_setup->refFreq;

	I2C_Init(i2c, &i2c_values);

	i2c->ROUTELOC0 = i2c_setup->scl_route | i2c_setup->sda_route;
	i2c->ROUTEPEN = i2c_setup->scl_enable | i2c_setup->sda_enable;
	i2c_bus_reset(i2c, i2c_info);

	i2c->IFC = I2C_IFC_NACK | I2C_IFC_ACK | I2C_IFC_MSTOP;
	i2c->IEN = I2C_IEN_NACK | I2C_IEN_ACK | I2C_IEN_MSTOP | I2C_IEN_RXDATAV;

	if(i2c == I2C0){
		NVIC_EnableIRQ(I2C0_IRQn);
	}
	else if(i2c == I2C1){
		NVIC_EnableIRQ(I2C1_IRQn);
	}

}

/***************************************************************************//**
 * @brief
 * 	This function reset the I2C bus protocol
 *
 * @details
 * 	This function keeps the SDA pin high while clearing and setting the SCL pin 9 times
 *
 * @note
 * 	Necessary for continuous communication
 *
 *
 ******************************************************************************/
void i2c_bus_reset(I2C_TypeDef *i2c, I2C_IO_STRUCT *i2c_info){
	EFM_ASSERT(GPIO_PinInGet(i2c_info->sda_port, i2c_info->sda_pin));
	EFM_ASSERT(GPIO_PinInGet(i2c_info->scl_port, i2c_info->scl_pin));
	GPIO_PinOutSet(i2c_info->sda_port,i2c_info->sda_pin);
	for(int i = 0; i < 9; i++){
		GPIO_PinOutClear(i2c_info->scl_port,i2c_info->scl_pin);
		GPIO_PinOutSet(i2c_info->scl_port,i2c_info->scl_pin);
	}
	GPIO_PinOutSet(i2c_info->scl_port,i2c_info->scl_pin);
	i2c->CMD = I2C_CMD_ABORT;

}

/***************************************************************************//**
 * @brief
 * 	This function begins the I2C communication
 *
 * @details
 * 	It first fills in the necessary information into the I2C _PAYLOAD struct
 * 	then begins the process of the software flow chart for communicating with a
 * 	sensor
 *
 *
 * @note
 * 	This function will work with either for I2C0 or I2C1
 *
 ******************************************************************************/
void i2c_start(I2C_TypeDef *i2c, uint32_t device_address, uint32_t device_register, uint32_t *data, int bytes){
	// Fill in payload struct
	payload.i2c = i2c;
	payload.device_address = device_address;
	payload.bytes = bytes;
	payload.device_register = device_register;
	payload.data = data;

	// Assert idle
	EFM_ASSERT((i2c->STATE & _I2C_STATE_STATE_MASK) == I2C_STATE_STATE_IDLE);

	// Block Sleep Mode
	sleep_block_mode(I2C_EM_BLOCK);

	// Start Ladder Diagram
	payload.state = start;
	payload.i2c->CMD = I2C_CMD_START;
	////
	payload.i2c->TXDATA = I2C_ADDR_W(payload.device_address);
}

/***************************************************************************//**
 * @brief
 * 	This is the IRQHandler for I2C0
 *
 * @details
 * 	Standard IRQHandler that clears the interrupt flag and then services the given interrupt
 *
 * @note
 * 	This function is for I2C0
 *
 *
 ******************************************************************************/
void I2C0_IRQHandler(){
	EFM_ASSERT(payload.i2c == I2C0);
	uint32_t int_flag;
	int_flag = I2C0->IF & I2C0->IEN;
	I2C0->IFC = int_flag;
	if(int_flag & I2C_IF_ACK){
		EFM_ASSERT(!(I2C0->IF & I2C_IF_ACK));
		i2c_ack();
	}
	if(int_flag & I2C_IF_NACK){
		EFM_ASSERT(!(I2C0->IF & I2C_IF_NACK));
		i2c_nack();
	}
	if(int_flag & I2C_IF_MSTOP){
		EFM_ASSERT(!(I2C0->IF & I2C_IF_MSTOP));
		i2c_mstop();
	}
	if(int_flag & I2C_IF_RXDATAV){
		i2c_rxdatav();
	}
}

/***************************************************************************//**
 * @brief
 *	This is the IRQHandler for I2C1
 *
 * @details
 *	Standard IRQHandler that clears the interrupt flag and then services the given interrupt
 *
 * @note
 *	This function is for I2C1
 *
 ******************************************************************************/
void I2C1_IRQHandler(){
	EFM_ASSERT(payload.i2c == I2C1);
	uint32_t int_flag;
	int_flag = I2C1->IF & I2C1->IEN;
	I2C1->IFC = int_flag;
	if(int_flag & I2C_IF_ACK){
		EFM_ASSERT(!(I2C1->IF & I2C_IF_ACK));
		i2c_ack();
	}
	if(int_flag & I2C_IF_NACK){
		EFM_ASSERT(!(I2C1->IF & I2C_IF_NACK));
		i2c_nack();
	}
	if(int_flag & I2C_IF_MSTOP){
		EFM_ASSERT(!(I2C1->IF & I2C_IF_MSTOP));
		i2c_mstop();
	}
	if(int_flag & I2C_IF_RXDATAV){
		i2c_rxdatav();
	}
}


/***************************************************************************//**
 * @brief
 * 	State machine for when an ack interrupt occurs
 *
 * @details
 *	This function acts as a state machine for the ack interupt and asserts false for all
 *	states that aren't supposed to have an ack. It will transition states, then perform the
 *	actions of the following state.
 *
 * @note
 * Function is called by the IRQHandler
 *
 *
 ******************************************************************************/
static void i2c_ack(){
	switch(payload.state){
		case(start):{
			payload.state = measure_cmd;
			payload.i2c->TXDATA = payload.device_register;
			break;
		}
		case(measure_cmd):{
			payload.state = request_data;
			payload.i2c->CMD = I2C_CMD_START;
			payload.i2c->TXDATA = I2C_ADDR_R(payload.device_address);
			break;
		}
		case(request_data):{
			payload.state = read_MS;
			break;
		}
		case(read_MS):{
			EFM_ASSERT(false);
			break;
		}
		case(read_LS):{
			EFM_ASSERT(false);
			break;
		}
		case(stop):{
			EFM_ASSERT(false);
			break;
		}
		default:{
			EFM_ASSERT(false);
			break;
		}
	}

}

/***************************************************************************//**
 * @brief
 * State machine for when an nack interrupt occurs
 *
 * @details
 *	This function acts as a state machine for the nack interupt and asserts false for all
 *	states that aren't supposed to have a nack. It will transition states, then perform the
 *	actions of the following state.
 *
 * @note
 *	Function is called by the IRQHandler
 *
 ******************************************************************************/
static void i2c_nack(){
	switch(payload.state){
		case(start):{
			EFM_ASSERT(false);
			break;
		}
		case(measure_cmd):{
			EFM_ASSERT(false);
			break;
		}
		case(request_data):{
			payload.state = request_data;
			payload.i2c->CMD = I2C_CMD_START;
			payload.i2c->TXDATA = I2C_ADDR_R(payload.device_address);
			break;
		}
		case(read_MS):{
			EFM_ASSERT(false);
			break;
		}
		case(read_LS):{
			EFM_ASSERT(false);
			break;
		}
		case(stop):{
			EFM_ASSERT(false);
			break;
		}
		default:{
			EFM_ASSERT(false);
			break;
		}
	}

}

/***************************************************************************//**
 * @brief
 *	State machine for when an rxdatav interrupt occurs
 *
 * @details
 *	This function acts as a state machine for the rxdatav interupt and asserts false for all
 *	states that aren't supposed to have an rxdatav. It will transition states, then perform the
 *	actions of the following state.
 *
 * @note
 *	Function is called by the IRQHandler
 *
 ******************************************************************************/
static void i2c_rxdatav(){
	switch(payload.state){
		case(start):{
			EFM_ASSERT(false);
			break;
		}
		case(measure_cmd):{
			EFM_ASSERT(false);
			break;
		}
		case(request_data):{
			EFM_ASSERT(false);
			break;
		}
		case(read_MS):{
			*payload.data = (payload.i2c->RXDATA << 8);
			payload.i2c->CMD = I2C_CMD_ACK;
			payload.state = read_LS;
			break;
		}
		case(read_LS):{
			*payload.data |= payload.i2c->RXDATA;
			payload.i2c->CMD = I2C_CMD_NACK;
			payload.i2c->CMD = I2C_CMD_STOP;
			payload.state = stop;
			break;
		}
		case(stop):{
			EFM_ASSERT(false);
			break;
		}
		default:{
			EFM_ASSERT(false);
			break;
		}
	}

}

/***************************************************************************//**
 * @brief
 *	State machine for when an mstop interrupt occurs (master stop)
 *
 * @details
 *	This function acts as a state machine for the mstop interupt and asserts false for all
 *	states that aren't supposed to have an mstop. It will transition states, then perform the
 *	actions of the following state.
 *
 * @note
 *	Function is called by the IRQHandler
 *
 ******************************************************************************/
static void i2c_mstop(){
	switch(payload.state){
		case(start):{
			EFM_ASSERT(false);
			break;
		}
		case(measure_cmd):{
			EFM_ASSERT(false);
			break;
		}
		case(request_data):{
			EFM_ASSERT(false);
			break;
		}
		case(read_MS):{
			EFM_ASSERT(false);
			break;
		}
		case(read_LS):{
			EFM_ASSERT(false);
			break;
		}
		case(stop):{
			sleep_unblock_mode(I2C_EM_BLOCK);
			add_scheduled_event(I2C_DONE_EVT);
			payload.state = stop;
			break;
		}
		default:{
			EFM_ASSERT(false);
			break;
		}
	}

}


