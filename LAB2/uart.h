#include "stm32f10x.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

void USART_Init ();

void USART_Send (unsigned char symbol);

void USART_Receive (unsigned char *data, char *is_ready);

char USART_IsReady();

void USART_SendMessage(unsigned char *data);