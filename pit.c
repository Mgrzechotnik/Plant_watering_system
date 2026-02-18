#include "MKL05Z4.h"
#include	"pit.h"



void PIT_Init(void)
{
	uint32_t tsv;
	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;			
	PIT->MCR &= ~PIT_MCR_MDIS_MASK;				
	tsv=BUS_CLOCK;											
	//tsv=BUS_CLOCK/2;											
	//tsv=BUS_CLOCK/10;										
	//tsv=BUS_CLOCK/100;									
	PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(tsv);		
	PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TEN_MASK;		
}
