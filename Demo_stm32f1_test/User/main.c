/**
 * @file: main.c
 * @author: https://github.com/AIoTFamily
 * @brief: xx module is used to xxx
 * @copyright: Copyright (c) xxx
 **/

#include "stm32f10x.h"
#include "mem_manage.h"


MemHeapRegion_t xHeapRegions[] =
{
	{( uint8_t * ) 0x20004000UL, 0x1000 }, // << Defines a block of 0x10000 bytes starting at address 0x80000000
	{ NULL, 0 }                            // << Terminates the array.
};

static void __NVIC_CONFIG(void)
{
#ifdef ENABLE_BOOT
    NVIC_SetVectorTable(NVIC_VectTab_FLASH,OFFSET_FIRMWARE_L);
#else
    NVIC_SetVectorTable(NVIC_VectTab_FLASH,0);
#endif
    //Configure the NVIC Preemption Priority Bits
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
}

static void __rcc_clock_config(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);  
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    if(SysTick_Config(SystemCoreClock / 1000)){ 
        /* Capture error */ 
        while (1);
    }
}

static void mem_manage_test(void)
{
    uint8_t *(user[10]) = {NULL};
    uint32_t i = 0;

    for ( i = 0; i < 10; i++)
    {
        user[i] = (uint8_t *)pvPortMalloc((i+1) << 2);
		user[i] = (uint8_t *)pvPortMalloc((i+1) << 2);
    }

    for ( i = 0; i < 10; i += 2)
    {
        vPortFree(user[i]);
    }
}

int main(void)
{
    __NVIC_CONFIG();
    __rcc_clock_config();

    vPortDefineHeapRegions(xHeapRegions);
	
    mem_manage_test();
	
	SEGGER_RTT_printf(0,"SEGGER_RTT_printf success.\n");

    while(1)
    {
		SysTick_delay_ms(1000);
		xMemDebugPrintfFreeBlock();
    }
}


/* https://github.com/AIoTFamily */
