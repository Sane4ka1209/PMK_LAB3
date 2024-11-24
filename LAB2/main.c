#include "stm32f10x.h"
#include "ds18b20.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t LED_pin = 5;
#define MAX_SENSORS 2
Sensor sensors[MAX_SENSORS];

unsigned char symbol = 0;
unsigned char message[100];
unsigned char got[6] = "GOT:  ";
int count = 0;
char received = 0;
char IS_READY = 0;
uint32_t ticks = 0;

void TIM4_Init () {
		// Включаем таймер
		RCC -> APB1ENR |= RCC_APB1ENR_TIM4EN;
		// Включаем счетчик таймера
		TIM4 -> CR1 = TIM_CR1_CEN;
		
		// Настраиваем значение с которого таймер будет отсчитывать до 0 и вызывать прерывание
		// Нам нужно отталкиваться от настроенной частоты, чтобы прерывание срабатывало каждую секунду
		TIM4->PSC = SystemCoreClock / 10 / 1000 - 1;
		// Зачем так делать
		TIM4->ARR = 1000 - 1;

		// Разрешаем прерывания
		TIM4->DIER |= TIM_DIER_UIE;
		NVIC_EnableIRQ (TIM4_IRQn);
}

void SystemCoreClockConfigure(void) {
		RCC->CR |= ((uint32_t)RCC_CR_HSEON);                    
		while ((RCC->CR & RCC_CR_HSERDY) == 0);                  

		RCC->CFGR = RCC_CFGR_SW_HSE;                             
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE);  
	
	  RCC->CFGR = RCC_CFGR_HPRE_DIV1;                          // HCLK = SYSCLK
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV1;                        // APB1 = HCLK/2
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;                        // APB2 = HCLK/2

		RCC->CR &= ~RCC_CR_PLLON;                                

		//  PLL configuration:  = HSE * 9 = 72 MHz
		RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);
		RCC->CFGR |=  (RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);

		RCC->CR |= RCC_CR_PLLON;                                
		while((RCC->CR & RCC_CR_PLLRDY) == 0) __NOP();           

		RCC->CFGR &= ~RCC_CFGR_SW;                               
		RCC->CFGR |=  RCC_CFGR_SW_PLL;
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);  
}

void enable_pin(uint32_t pin_number){
		// Половина портов на CRL, другая на  CRH
		if(pin_number < 8ul){
				GPIOA->CRL &= ~((15ul << 4*pin_number));
				GPIOA->CRL |= (( 1ul << 4*pin_number));
		}
		else{
				pin_number -= 8ul;
				GPIOA->CRH &= ~((15ul << 4*pin_number));
				GPIOA->CRH |= (( 1ul << 4*pin_number));
		}
}
uint8_t check_pin(uint32_t pin_number){
		return (GPIOA->ODR & (1ul << pin_number));
}
void switch_pin (uint32_t pin_number) {
		if(GPIOA->ODR & (1ul << pin_number)){
			GPIOA->BSRR = 1ul << (pin_number + 16ul);
		}
		else{
			GPIOA->BSRR = 1ul << pin_number;
		}
}

void TIM4_IRQHandler () {
		TIM4->SR &= ~TIM_SR_UIF;
		uint8_t count = 0;
		for (uint8_t i = 0; i < MAX_SENSORS; i ++){
				if (sensors[i].temp > 25)
					count++;
		}
		if (count || check_pin(LED_pin))
			switch_pin (LED_pin);
}

void GPIO_Init (void) {
		// Разрешаем тактирование порта A
		RCC->APB2ENR |= (1UL << 2);
		
		// Определяем пин PA13 как выходной
		enable_pin(LED_pin);
}

void USART2_IRQHandler () {
		if (USART2->SR & USART_SR_IDLE) {
				received = 1;
				for (int i = 0; i < count; i++) {
							USART_Send(message[i]);
							message[i] = 0;
				}
				count = 0;
				(void)USART2->SR;
				(void)USART2->DR;
		}
	
}


void Init_Sensors () {													// reset all values in sensors
		for (uint8_t i = 0; i < MAX_SENSORS; i++)
		{
				sensors[i].raw_temp = 0x0;
				sensors[i].temp = 0.0;
				sensors[i].crc8_rom = 0x0;
				sensors[i].crc8_data = 0x0;
				sensors[i].crc8_rom_error = 0x0;
				sensors[i].crc8_data_error = 0x0;
				for (uint8_t j = 0; j < 8; j++)
				{
						sensors[i].ROM_code[j] = 0x00;
				}
				for (uint8_t j = 0; j < 9; j++)
				{
						sensors[i].scratchpad_data[j] = 0x00;
				}
		}
}

void TIM3_Init () {
		// Включаем таймер
		RCC -> APB1ENR |= RCC_APB1ENR_TIM3EN;
		// Включаем счетчик таймера
		TIM3 -> CR1 = TIM_CR1_CEN;
		
		// Настраиваем значение с которого таймер будет отсчитывать до 0 и вызывать прерывание
		// Нам нужно отталкиваться от настроенной частоты, чтобы прерывание срабатывало каждую секунду
		TIM3->PSC = SystemCoreClock / 100 / 1000 - 1;
		// Зачем так делать
		TIM3->ARR = 1000 - 1;

		// Разрешаем прерывания
		TIM3->DIER |= TIM_DIER_UIE;
		NVIC_EnableIRQ (TIM3_IRQn);
}

void TIM3_IRQHandler () {
		TIM3->SR &= ~TIM_SR_UIF;
		if (IS_READY){
			ticks++;
			if (ticks > 1000){
				IS_READY = 0;
				ticks = 0;
			}
		}
}

int main(void)
{			
			SystemCoreClockConfigure();
			SystemCoreClockUpdate();
			SysTick_Config(SystemCoreClock / 1000000);
			GPIO_Init();
			USART_Init();
			
			
			uint8_t devCount = 0, i = 0;	
			TIM4_Init();		
			TIM3_Init();
			ds18b20_PortInit();																																																			//	Init PortB.11 for data transfering 
			//while (ds18b20_Reset()) ;																																																// 	If DS18B20 exists then continue
						
			devCount = Search_ROM(0xF0,&sensors);	
			
			
			
			while(1){
				for (i = 0; i < devCount; i++) {
							if (!sensors[i].crc8_data_error) {
									ds18b20_ConvertTemp(1, sensors[i].ROM_code);																																// 	Measure Temperature (Send Convert Temperature Command)
							}
							DelayMicro(750000); 																																														// 	Delay for conversion (max 750 ms for 12 bit)
							ds18b20_ReadStratchpad(1, sensors[i].scratchpad_data, sensors[i].ROM_code); 
							sensors[i].crc8_data = Compute_CRC8(sensors[i].scratchpad_data,8);																							// 	Compute CRC for data
							sensors[i].crc8_data_error = Compute_CRC8(sensors[i].scratchpad_data,9) == 0 ? 0 : 1;														// 	Get CRC Error Signal
							if (!sensors[i].crc8_data_error) {																																							// 	Check if data correct
									sensors[i].raw_temp = ((uint16_t)sensors[i].scratchpad_data[1] << 8) | sensors[i].scratchpad_data[0]; 			// 	Get 16 bit temperature with sign
									sensors[i].temp = sensors[i].raw_temp * 0.0625;																															// 	Convert to float
							}							
							DelayMicro(100000);
							WriteTHTL(&sensors[i]);
							WriteScratchpad(&sensors[i], 1);
							CopyScratchpad(&sensors[i], 1);
						}
				
				
				
				IS_READY = 1;
				USART_SendMessage("R ");
				USART_Receive(&symbol, &IS_READY);
				
				if(symbol != 0){
					got[4] = symbol;
					USART_SendMessage(got);
					if (0 == symbol - 0x30 || symbol - 0x30 > MAX_SENSORS){
						USART_SendMessage("Undef\t");
					} else {
						float temp = sensors[symbol - 0x31].temp;
						USART_SendMessage("Temp:");
						
						size_t needLen = snprintf(NULL, 0, "%f", temp);
						unsigned char* tempT = malloc(needLen);
						snprintf(tempT, needLen, "%f", temp);
						
						USART_SendMessage(tempT);
						
						USART_Send('\t');
					}
					symbol = 0;
					ticks = 0;
				} else {
					USART_SendMessage("NR ");
				}
			}
}