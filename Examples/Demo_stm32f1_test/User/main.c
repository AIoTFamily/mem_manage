/**
 * @file: main.c
 * @author: https://github.com/AIoTFamily
 * @brief: xx module is used to xxx
 * @copyright: Copyright (c) xxx
 **/

#include "stm32f10x.h"
#include "mem_manage.h"
#include <time.h>

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
 
/************** 内存碎片测试程序 ************
 * 利用 rand 产生指定范围内的伪随机数，然后申请对应大小的内存
 * 释放内存时，也随机释放
*******************************************/
#define TEST_BLOCK_NUM  210
static uint8_t *(user[TEST_BLOCK_NUM + 1]) = {NULL};

static uint32_t __build_rand_value(void)
{
	uint32_t rand_value = 0;
	rand_value = rand() % TEST_BLOCK_NUM;
	if(rand_value == 0)
		rand_value = rand_value + 1;
	
	return rand_value;
}

static uint32_t calcate_used_mem_size(void)
{
    uint32_t i = 0,usedMemSize = 0;
    for ( i = 0; i < TEST_BLOCK_NUM + 1; i++){
        if (user[i] != NULL){
            usedMemSize += i;
        }
    }
    return usedMemSize;
}

static int mem_manage_test(void)
{
    uint32_t rand_value = 0, size = 0;
    static uint32_t count = 0;

    if (count == 0){
        srand(125);
        count = 1;
    }

    rand_value = __build_rand_value();

    size = rand_value;
    if (user[rand_value] == NULL){
        user[rand_value] = (uint8_t *)MEM_MALLOC(size);
        if (user[rand_value] == NULL){
            return -1;
        }
    }

    rand_value = __build_rand_value();
    if (user[rand_value] != NULL){
        MEM_FREE(user[rand_value]);
        user[rand_value] = NULL;
    }

    return 0;
}

static void malloc_fail_handle(size_t xWantedSize)
{
    SEGGER_RTT_printf(0,"malloc_fail_handle,xWantedSize:%d\n",xWantedSize);
}

static uint8_t userHeap[0x4000] __attribute__((at(0x20001000)));
static MemHeapRegion_t xHeapRegions[] ={
	{( uint8_t * )userHeap, sizeof(userHeap)}, // << Defines a block of 0x10000 bytes starting at address 0x80000000
	{ NULL, 0 }                                // << Terminates the array.
};

int main(void)
{
    uint32_t freeMemBlockMaxNum = 0, freeNum = 0, testCount = 0;
    uint32_t usedMemSize = 0, freeMemSize = 0;

    mem_manage_t mem_manage;
    memset(&mem_manage,0,sizeof(mem_manage_t));
    mem_manage.malloc_fail_cb = malloc_fail_handle;
    memManageFunctionInit(&mem_manage,xHeapRegions);

    __NVIC_CONFIG();
    __rcc_clock_config();

	SEGGER_RTT_printf(0,"mem test start.....\n");

    while(1)
    {
		// SysTick_delay_ms(5);
		
        freeNum = memGetFreeBlockNum();
		if( freeNum > freeMemBlockMaxNum ){
            freeMemBlockMaxNum = freeNum;
        }

        if( mem_manage_test() != 0){
            SEGGER_RTT_printf(0,"test complete,count:%d.\n",testCount);
            usedMemSize = calcate_used_mem_size();
            freeMemSize = memGetFreeHeapSize();
            SEGGER_RTT_printf(0,"used size:%d,free size:%d,usage rate:%d%c\n",usedMemSize,freeMemSize,\
                                (usedMemSize * 100) / (freeMemSize + usedMemSize),37);
            memPrintfFreeListLayout();
            break;
        }
		testCount++;
    }


}


/* https://github.com/AIoTFamily */
