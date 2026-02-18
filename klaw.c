#include "klaw.h"
#include "frdm_bsp.h"

volatile uint8_t S2_press=0;	
volatile uint8_t S3_press=0;
volatile uint8_t S4_press=0;

void Klaw_Init(void)
{
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;		
	PORTA->PCR[S1] |= PORT_PCR_MUX(1);
	PORTA->PCR[S2] |= PORT_PCR_MUX(1);
	PORTA->PCR[S3] |= PORT_PCR_MUX(1);
	PORTA->PCR[S4] |= PORT_PCR_MUX(1);
	PORTA->PCR[S1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[S2] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[S3] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[S4] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
}
void Klaw_S2_4_Int(void)
{
	PORTA -> PCR[S2] |= PORT_PCR_IRQC(0xa);		
	PORTA -> PCR[S3] |= PORT_PCR_IRQC(0xa);		
	PORTA -> PCR[S4] |= PORT_PCR_IRQC(0xa);
	NVIC_SetPriority(PORTA_IRQn, 3); 
	NVIC_ClearPendingIRQ(PORTA_IRQn);
	NVIC_EnableIRQ(PORTA_IRQn);
}

void PORTA_IRQHandler(void)	
{
	uint32_t buf;
	buf=PORTA->ISFR & (S2_MASK | S3_MASK | S4_MASK);

	switch(buf)
	{
		case S2_MASK:	DELAY(100)		
									if(!(PTA->PDIR&S2_MASK))		
									{
										if(!S2_press)
										{
											S2_press=1;
										}
									}									
									break;
		case S3_MASK:	DELAY(100)		
									if(!(PTA->PDIR&S3_MASK))		
									{
										if(!S3_press)
										{
											S3_press=1;
										}
									}
									break;
		case S4_MASK:	DELAY(100)		
									if(!(PTA->PDIR&S4_MASK))		
									{
										if(!S4_press)
										{
											S4_press=1;
										}
									}
									break;
		default:			break;
	}	
	PORTA->ISFR |=  S2_MASK | S3_MASK | S4_MASK;	
	NVIC_ClearPendingIRQ(PORTA_IRQn);
}