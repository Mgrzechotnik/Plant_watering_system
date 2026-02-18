#include "MKL05Z4.h"
#include "lptmr.h"

void LPTMR_Init(uint16_t count_val){
	SIM->SCGC5 |= SIM_SCGC5_LPTMR_MASK; // turn on clock
	LPTMR0->CSR = 0; //clear all setings
	
	LPTMR0->PSR = LPTMR_PSR_PRESCALE(3) | LPTMR_PSR_PCS(1); //prescaler 3 -> 2^3+1 = 2^14 = 16 and interior oscillator LPO LOW power oscillator 1kHz
	
	LPTMR0->CMR = count_val; //compare register
	LPTMR0->CSR |= LPTMR_CSR_TIE_MASK; //signal interrupt
	
	NVIC_ClearPendingIRQ(LPTimer_IRQn); // clear flags of interupts
	NVIC_EnableIRQ(LPTimer_IRQn); // if interrupt comes, stop doing now what you do and go to handler
	
	LPTMR0->CSR |= LPTMR_CSR_TEN_MASK; // timer on
	
}

void LPTimer_IRQHandler(void){
	LPTMR0->CSR |= LPTMR_CSR_TCF_MASK; // confimation of interrupt
}
