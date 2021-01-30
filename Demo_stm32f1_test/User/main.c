/**
 * @file: main.c
 * @author: https://github.com/AIoTFamily
 * @brief: xx module is used to xxx
 * @copyright: Copyright (c) xxx
 **/

#include "stm32f10x.h"
#include "mem_manage.h"


static uint8_t userHeap[0x4000] __attribute__((at(0x20001000)));
MemHeapRegion_t xHeapRegions[] =
{
	{( uint8_t * )userHeap, sizeof(userHeap)}, // << Defines a block of 0x10000 bytes starting at address 0x80000000
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
 
/************** 内存碎片测试程序 ************
 * 利用 rand 产生指定范围内的伪随机数，然后申请对应大小的内存
 * 释放内存时，也随机释放
*******************************************/
#define TEST_BLOCK_NUM  210
static uint8_t *(user[TEST_BLOCK_NUM + 1]) = {NULL};
static uint32_t usedMemSize = 0,freeMemSize = 0;

static uint32_t __build_rand_value(void)
{
	uint32_t rand_value = 0;
	rand_value = rand() % TEST_BLOCK_NUM;
	if(rand_value == 0)
		rand_value = rand_value + 1;
	
	return rand_value;
}

static int mem_manage_test(void)
{
    uint32_t rand_value = 0, size = 0;

    rand_value = __build_rand_value();

    size = rand_value;
    if (user[rand_value] == NULL){
        user[rand_value] = (uint8_t *)MEM_MALLOC(size);
        if (user[rand_value] == NULL){
            SEGGER_RTT_printf(0,"MEM_MALLOC fail, want size:%d\n",size);
            return -1;
        }else{
            usedMemSize += size;
        }
    }

    rand_value = __build_rand_value();
    if (user[rand_value] != NULL){
        MEM_FREE(user[rand_value]);
        user[rand_value] = NULL;
        freeMemSize += (rand_value);
    }

    return 0;
}

int main(void)
{
    uint32_t freeMemBlockMaxNum = 0, freeNum = 0, testCount = 0;
    /* 初始化随机数发生器 */
    srand(125);
    __NVIC_CONFIG();
    __rcc_clock_config();

    vPortDefineHeapRegions(xHeapRegions);

	SEGGER_RTT_printf(0,"mem test start.....\n");

    while(1)
    {
		// SysTick_delay_ms(5);
		
        freeNum = xMemGetFreeBlockNum(0);
		if( freeNum > freeMemBlockMaxNum ){
            freeMemBlockMaxNum = freeNum;
        }

        if( mem_manage_test() != 0){
            SEGGER_RTT_printf(0,"test complete,count:%d,freeMemBlockMaxNum:%d.\n",testCount,freeMemBlockMaxNum);
            SEGGER_RTT_printf(0,"used size:%d, free size:%d\n",usedMemSize,freeMemSize);

            xMemGetFreeBlockNum(1);
            break;
        }
		testCount++;
    }


}


/* https://github.com/AIoTFamily */
