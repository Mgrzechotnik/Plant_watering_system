/**
 * @brief Plant watering system
 *-----------------------------------------------------
					Technika Mikroprocesorowa 2 - projekt
					Plant Watering system
					autorzy: Hubert Korzonek, Grzegorz Mróz
					wersja: 23.01.2026
 *-----------------------------------------------------
 * PINOUT:
 * LIGHT SENSOR: PTB11, VCC -> 3.3
 * SOIL SENSOR: PTB10, VCC -> 3.3
 * LCD: SDA -> PTB4, SCL -> PTB3
 * MOSFET KEY: PTB7
 * SPEAKER: PTB6
 * KEYPAD: C4 -> PTA8, C3 -> PTA10, C2 -> PTA11, C1 -> PTA12, R4 -> GND
 * 
 */
#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "lcd1602.h"
#include "klaw.h"
#include "TPM.h"
#include "ADC.h"
#include "pit.h"
#include "lptmr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define TICKS_TEST_10S 625 //Oscilator 1kHz, Prescaler 16 -> 62,5Hz. LTPMR count +1 1/62,5 Hz -> 0,016 s. 625 cycles -> 10 seconds.
#define WAKEUP_TICKS TICKS_TEST_10S
#define VOLTAGE_DRY 2.905f
#define VOLTAGE_WET 1.320f
#define MINUTES_ADD 90 // Ticks Test to real time prescaler

//********* Speaker code from lab

#define MOD_MAX	40082
#define ZEGAR 1310720
#define DIV_CORE	16

float	ampl_v;
float freq;
uint16_t	mod_curr=MOD_MAX;
uint16_t	ampl;
uint16_t Tony[]={40082, 37826, 35701, 33702, 31813, 30027, 28339, 26748, 25249, 23830, 22493, 21229, 20041};
uint16_t Oktawa[]={1, 2, 4, 8, 16, 32, 64, 128};
uint8_t	Kotek[]={7,4,4,5,2,2,0,4,7,7,4,4,5,2,2,0,4,0};	// Wysokość nuty
uint8_t	Kotek_W[]={4,4,4,4,4,4,2,2,8,4,4,4,4,4,4,2,2,8};	// Czas trwania dźwięku
uint8_t Kotek_Gap[]={2,2,2,2,2,2,1,1,4,2,2,2,2,2,2,1,1,10};		// Czas trwania przerwy
uint8_t	Fuga[]={9,7,9,7,5,4,2,1,2,9,7,9,4,5,1,2};
uint8_t	Fuga_W[]={2,2,16,2,2,2,2,4,16,2,2,16,4,4,4,16};
uint8_t Fuga_Gap[]={1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,10};
uint8_t	ton=0;
int8_t	gama=2;
uint8_t	gap_ton=0;
uint8_t on_off=255;

//*********



volatile float sum;

volatile uint8_t hour = 18;
volatile uint16_t sysminutes = 0;
volatile uint8_t minutes = 40;

/**
 * @brief light sensor variables
 */

uint8_t avg_calculated = 0;
float light_sum = 0.0f;
float daily_avg_light = 0.0f;
float avg = 0.0f;
uint32_t light_count = 0;
volatile uint8_t not_enough_sun = 0;

/**
 * @brief watering varaibles
 */

volatile uint16_t water_ml = 50;
volatile uint16_t water_time = 0;


/**
 * @brief SysTick variables
 */

volatile uint32_t czas=1;				
volatile uint8_t sekunda=0;			
volatile uint8_t sekunda_OK=0;


char display[32];//={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};

/**
 * @brief soil logic
 */
 
uint16_t soil_moisture[5] = {10, 15, 20, 25, 30};
uint8_t pos = 0;
uint8_t days = 1;
uint8_t selected_soil_moisture = 0;
volatile uint8_t selected_watering_frequency = 0;

/**
 * @brief ADC calibration coefficent
 * 2.91V/ADC Resolution 4096
 */

volatile float adc_volt_coeff = ((float)(((float)2.91) / 4096) );			
uint8_t	kal_error;


/**
 * @brief function to generate delay
 * Force processor to do not necessary operations
 * Tproc = 21 ns. 1 ms is 48000 cycles. 48000 / 4000 = 12 cycles per one loop.
 * Costs: LDR 2, NOP 1, ADDS 1, CMP 1, STR 2, BLT 3, Wait states 2.
 * 
 */
void delay_12(uint32_t ms){
	volatile uint32_t i,j;
		
	for( i = 0; i < ms; i++){
		for(j = 0; j < 4000; j++){
			__NOP();
		}
	}
	
}

/**
 * @brief Displays the menu to set watering value
 */
void set_water_ml(){
	LCD1602_ClearAll();
	LCD1602_Print("Set water ml");
	while(1){
		LCD1602_SetCursor(0,1);
		sprintf(display,"Set ml: %3d",water_ml);
		LCD1602_Print(display);
		
		if(S2_press){
			water_ml+=50;
			if(water_ml > 500){
				water_ml = 50;
			}
			S2_press = 0;
		}
		if(S3_press){
			S3_press = 0;
			break;
		}
}
}
/**
 * @brief water pump power control by PWM
 * @param power_percent Given by user power in percent for pump
 */
void set_pump_power(uint8_t power_percent){
	if(power_percent > 100){
		power_percent = 100;
	}
	TPM0->MOD = 0xFFFF;
	uint32_t value = power_percent * 65535/100;
	TPM0->CONTROLS[2].CnV = value;
}
/**
 * @brief Measures soil moisture level 
 * using ADC, masure voltage and calculate percenage of soil moisture
 * @return soil moisture in percentage
 */
uint8_t soil_moisture_measurment(){
	ADC0->SC1[0] = ADC_SC1_ADCH(9);
	while(!(ADC0->SC1[0] & ADC_SC1_COCO_MASK));
	volatile uint16_t soil = ADC0->R[0];
	volatile float voltage = soil*adc_volt_coeff;
	
	if (voltage >= VOLTAGE_DRY){
		return 0;
	}
	if(voltage <= VOLTAGE_WET){
		return 100;
	}
	
	float range = VOLTAGE_DRY - VOLTAGE_WET;
	float delta = VOLTAGE_DRY - voltage;
	float result = (delta/range)*100.0f;
	
	return result;
	
}
/**
 * @brief measure ambient light voltage
 * @return float light sensor voltage
 */
float adc_light_voltage(){
	ADC0->SC1[0] = ADC_SC1_ADCH(8);
	while(!(ADC0->SC1[0] & ADC_SC1_COCO_MASK));
	volatile uint16_t temp = ADC0->R[0];
	volatile float result = temp*adc_volt_coeff;
	return result;
}
/**
 * @brief clear line on LCD display
 * @param row Given by user to clear
 */
void LCD1602_ClearLine(uint8_t row){
	LCD1602_SetCursor(0,row);
	LCD1602_Print("                ");
	LCD1602_SetCursor(0,row);
}
/**
 * @brief displays currnet watering frequency
 */
void show_watering_frequency_menu(){
	LCD1602_ClearAll();
	LCD1602_SetCursor(0,0);
	LCD1602_Print("Watering frequency");
	LCD1602_SetCursor(0,1);
	sprintf(display,"Freq: %2d",days);
	LCD1602_Print(display);
	LCD1602_Print(" Days");
	
}
/**
 * @brief displays currnet soil moisture values
 *
 */
void show_soil_moisture_menu(){
	
	LCD1602_SetCursor(0,0);
	LCD1602_Print("Soil moisture");
	LCD1602_SetCursor(0,1);
	sprintf(display,"%d%%",soil_moisture[pos]);
	LCD1602_Print(display);
}
/**
 * @brief menu for selecting soil moisture values
 * cycles through soil_moisture array using S2 button
 * S3 button for confirm
 */
void set_soil_moisture(){

	while(1){
		if(S3_press){
				selected_soil_moisture = soil_moisture[pos];
				S3_press = 0;
				break;
		}
		
		if(S2_press){
			
			pos++;
			if(pos % 5 == 0){
				pos = 0;
			}
			
			LCD1602_ClearLine(1);
			sprintf(display, "%d%%", soil_moisture[pos]);
			LCD1602_Print(display);
			S2_press = 0;
		}
		
	}
}
/**
 * @brief menu for selecting water frequency value
 * cycles from 1 to 15 days using S2 button
 * S3 button for confirm
 */
void set_watering_frequency(){
	while(1){
		
		if(S3_press){
				selected_watering_frequency = days;
				S3_press = 0;
				break;
		}
		
		if(S2_press){
			
			days++;
			if(days % 15 == 0){
				days = 1;
			}
			
			LCD1602_ClearLine(1);
			sprintf(display, "Freq: %2d", days);
			LCD1602_Print(display);
			LCD1602_Print(" Days");
			S2_press = 0;
		}		
	}
}
/**
 * @brief menu for setting the currnet system time
 * first set hour
 * then set minutes
 * S2 for increment, S3 for confirm
 */
void set_time_menu(){
	LCD1602_ClearAll();
	LCD1602_Print("Set start time:");
	while(1){
		LCD1602_SetCursor(0,1);
		sprintf(display,"Time: %02d:%02d ",hour,minutes);
		LCD1602_Print(display);
		
		if(S2_press){
			hour++;
			if(hour > 23){
				hour = 0;
			}
			S2_press = 0;
		}
		if(S3_press){
			S3_press = 0;
			break;
		}
	}
	while(1){
		LCD1602_SetCursor(0,1);
		sprintf(display,"Time: %02d:%02d ",hour,minutes);
		LCD1602_Print(display);
		
		if(S2_press){
			minutes++;
			if(minutes >= 60){
				minutes = 0;
			}
			S2_press = 0;
		}
		if(S3_press){
			S3_press = 0;
			break;
		}
	}
	LCD1602_ClearLine(1);
}
/**
 * @brief Function to update time.
 * The clock starts by the time given by user and for cycles of deep cycles and how long lasts one and add its value
 */
void update_time(){
	minutes += MINUTES_ADD;
	sysminutes +=MINUTES_ADD;
	while(minutes >= 60){
		minutes -= 60;
		hour++;
	}
	if(hour >= 24){
		hour = 0;
		//light_sum = 0;
		//light_count = 0;
		//avg_calculated = 0;
	}
	LCD1602_ClearLine(1);
}

//********* Speaker code from laboratory
void SysTick_Handler(void)	
{
	sekunda+=1;
	if(sekunda==czas)
	{
		switch(gap_ton)
		{
			case 0	:		czas=Fuga_W[ton];		
									mod_curr=Tony[Fuga[ton]]/Oktawa[gama];		
									TPM0->MOD = mod_curr;
									ampl=(int)mod_curr/50;
									TPM0->CONTROLS[3].CnV = ampl;	
									TPM0->SC |= TPM_SC_CMOD(1);
									sekunda=0;
									gap_ton=1;
									break;
			case 1	:		TPM0->CONTROLS[3].CnV = 0;
									sekunda=0;
									czas=Fuga_Gap[ton];	
									ton+=1;
									if(ton==16)		
										ton=0;
									gap_ton=0;
									break;
		}
	}
}
//***************

int main (void) 
{
	uint8_t w;
	

	//initialization of all things
	Klaw_Init();
	Klaw_S2_4_Int();
	LCD1602_Init();		 
	LCD1602_Backlight(TRUE);
	PWM_Init();				
	LCD1602_Print(display);
	LCD1602_ClearAll();
	
	//calibration 
	kal_error=ADC_Init();		
	if(kal_error)
	{
		while(1);							
	}
	
	//menu configuration
	set_time_menu();
	show_soil_moisture_menu();
	set_soil_moisture();
	show_watering_frequency_menu();
	set_watering_frequency();
	set_water_ml();
	
	LCD1602_ClearAll();
	LCD1602_Print("START");
	delay_12(2000);
	
	//Prepare for deep sleep, for minimalization current consuption
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	
	while(1){
		LCD1602_Backlight(TRUE);
		LCD1602_ClearAll();
		
		LCD1602_SetCursor(0,0);
		sprintf(display,"Time: %02d:%02d",hour,minutes);
		LCD1602_Print(display);
		delay_12(2000);
		LCD1602_ClearAll();
		
		//Calculate average light between 7 am to 7pm
		
		if(hour >= 7 && hour < 19){
			float light_voltage = adc_light_voltage();
			light_sum += light_voltage;
			light_count++;
			
			sprintf(display,"Light: %.2fV",light_voltage);
			LCD1602_Print(display);
			delay_12(2000);
			LCD1602_ClearAll();
			
			
		} else{
			if(light_count > 0){
				
				avg = light_sum/light_count;
				
				if(avg <= 0.50){
					not_enough_sun = 1;
					light_count = 0;
					light_sum =0;
				}else{
					light_count = 0;
					light_sum = 0;
				 }
				
			}
		 }
		/**
		 * @brief too low light level alarm with blinking led and buzzer
		 * exit with S3 button
		 */
        if(not_enough_sun){
            LCD1602_ClearAll();
            LCD1602_SetCursor(0,0);
            LCD1602_Print("Not enough");
            LCD1602_SetCursor(0,1);
            LCD1602_Print("sun for plant");
            
            PORTB->PCR[8] = PORT_PCR_MUX(1);
            PTB->PDDR |= (1UL << 8);
						SysTick_Config(SystemCoreClock/DIV_CORE);
            while(!S3_press){
                PTB->PCOR = (1UL << 8);
                delay_12(500);
                PTB->PSOR = (1UL << 8);
                delay_12(500);                
              if(S3_press){
                break;
                }
            }
						SysTick_Config(1);
						TPM0->CONTROLS[3].CnV = 0;
            PTB->PSOR = (1UL << 8);
            S3_press = 0;
            not_enough_sun = 0;
            LCD1602_ClearAll();
        }
	 
		//display current soil moisture
		uint8_t actual_soil_moisture = soil_moisture_measurment();
		LCD1602_ClearAll();
		LCD1602_SetCursor(0,0);
		sprintf(display,"Actual: %d%%",actual_soil_moisture);
		LCD1602_Print(display);
		//display selected soil moisture
		LCD1602_SetCursor(0,1);
		sprintf(display,"Prog: %d%%",selected_soil_moisture);
		LCD1602_Print(display);
		
		delay_12(2000);
		
		if((actual_soil_moisture < selected_soil_moisture) && sysminutes >= selected_watering_frequency*1440){
			sysminutes =0;
			LCD1602_ClearAll();
			LCD1602_Print("Watering");
			
			if((((float)water_ml/70.0)-(uint16_t)water_ml/70) >= 0.5){
				water_time = (uint32_t)water_ml*1000/70 + 1;
			} else {
					water_time = (uint32_t)water_ml*1000/70;
				}
			
			set_pump_power(100);
			delay_12(water_time);
			set_pump_power(0);
			
			LCD1602_SetCursor(0,1);
			LCD1602_Print("Finished watering");
			delay_12(1000);
			
		}
		else if(actual_soil_moisture < selected_soil_moisture){
			LCD1602_ClearAll();
			LCD1602_SetCursor(0,0);
			LCD1602_Print("Moisture:NOT OK");
			LCD1602_SetCursor(0,1);
			float hours_left = (selected_watering_frequency*1440 - sysminutes)/60.0;
			if(hours_left < 0){
				hours_left = 0;
			}
			sprintf(display,"Wait hours:%.2f",hours_left);
			LCD1602_Print(display);
			delay_12(3000);
		}
		else {
				LCD1602_ClearAll();
				LCD1602_Print("Soil moisture:OK");
				delay_12(2000);
			}
		
		set_pump_power(0);
		LCD1602_Backlight(FALSE);
		LCD1602_ClearAll();
		
		LPTMR_Init(WAKEUP_TICKS);
		
		__WFI();
		update_time();
	};

	
}
