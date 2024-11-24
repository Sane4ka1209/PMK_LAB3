#include "uart.h"
#include <string.h>

void USART_Init () {
	
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN; 
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; 
	
	// CNF2 = 10, MODE2 = 01
	GPIOA->CRL &= (~GPIO_CRL_CNF2_0); 
	GPIOA->CRL |= (GPIO_CRL_CNF2_1 | GPIO_CRL_MODE2);

	// CNF3 = 10, MODE3 = 00, ODR3 = 1
	GPIOA->CRL &= (~GPIO_CRL_CNF3_0);
	GPIOA->CRL |= GPIO_CRL_CNF3_1;
	GPIOA->CRL &= (~(GPIO_CRL_MODE3));
	GPIOA->BSRR |= GPIO_ODR_ODR3;

	//USARTDIV = SystemCoreClock / (16 * BAUD) = 72000000 / (16 * 9600) = 468,75

	USART2->BRR = 7500; 

	USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_IDLEIE ;
	USART2->CR2 = 0;
	USART2->CR3 = 0;
	NVIC_EnableIRQ(USART2_IRQn);
}
void USART_Send (unsigned char symbol) {
		while ((USART2->SR & USART_SR_TXE) == 0) {}
		USART2->DR = symbol;
}

char USART_IsReady(){
	if ((USART2->SR & USART_SR_RXNE) == 0){
		return 0;
		}
	return 1;
}

void USART_Receive (unsigned char *data, char *is_ready) {
		 while (!USART_IsReady() && *is_ready) {}
		if (*is_ready)
			*data = (unsigned char)USART2->DR;
}
void USART_SendMessage(unsigned char *data){
	uint8_t len = strlen(data);
		for (uint8_t i = 0; i < len; i++){
				USART_Send(data[i]);
		}
}