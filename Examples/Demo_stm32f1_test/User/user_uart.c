/**
 * @file: user_uart.c
 * @author: https://github.com/AIoTFamily
 * @brief: xx module is used to xxx
 * @copyright: Copyright (c) xxx
 **/

#include "stm32f10x.h"

uint8_t user_uart1_init(uint8_t baudrate)
{
	USART_InitTypeDef USART_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	// Enable GPIOA clocks 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	// Enable USART1 clocks 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	/*
	*  USART1_TX -> PA9 , USART1_RX -> PA10
	*/				
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	         
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
	GPIO_Init(GPIOA, &GPIO_InitStructure);		   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	        
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//
	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure); 
	//
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	USART_ITConfig(USART1, USART_IT_IDLE, DISABLE);
	//
	USART_ClearFlag(USART1,USART_FLAG_TC);

	//Enable the USART1 Interrupt
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;                             //Ö¸¶¨ÖÐ¶ÏÔ´
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;                     //ÇÀÕ¼ÓÅÏÈ¼¶
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;                            //Ö¸¶¨ÏìÓ¦ÓÅÏÈ¼¶±ð
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;                               //ÔÊÐíÖÐ¶Ï
	NVIC_Init(&NVIC_InitStructure);                                               //Ð´ÈëÉèÖÃ

	USART_Cmd(USART1, ENABLE);
  
	return 0;
}

void uart_putchar(uint8_t ch)
{
#ifdef DEBUG_IN_USART1
	//·¢ËÍ
	USART_SendData(USART1,ch);
	//
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
#else
	USART_SendData(USART1,ch);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
#endif
}

