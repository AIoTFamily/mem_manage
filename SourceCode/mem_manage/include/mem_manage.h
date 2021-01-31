/************************************
 * @file: mem_manage.c
 * @author: LinusZhao
 * @brief: 嵌入式裸机或RTOS系统下内存管理的实现
 * @version: 1.0.0
 * @date: 2021-01-01
 * @attention: 基本算法思想来源于 FreeRTOS heap5
 * 使用代码参考:
	static uint8_t userHeap[0x4000] __attribute__((at(0x20001000)));
	static MemHeapRegion_t xHeapRegions[] ={
		{( uint8_t * )userHeap, sizeof(userHeap)}, // << Defines a block of 0x10000 bytes starting at address 0x80000000
		{ NULL, 0 }                                // << Terminates the array.
	};

	static void malloc_fail_handle(size_t xWantedSize)
	{
		// 一般做重启系统处理
	}

	int main(void)
	{
		mem_manage_t mem_manage = {
			.malloc_fail_cb = malloc_fail_handle
		};
		memManageFunctionInit(&mem_manage,xHeapRegions);

	}
*************************************/

#ifndef __MEM_MANAGE_H__
#define __MEM_MANAGE_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#ifdef __cplusplus
    extern "C" {
#endif

#define MEM_MANAGE_VERSION		"1.0.0"

#define MEM_NO_HANDLE( x )
#define memBYTE_ALIGNMENT   			8

// #define MEM_MANAGE_PRINTF(fmt, ...)     printf(fmt, ##__VA_ARGS__)

#define TATTER_OPTIME_EN	1	// 碎片优化使能

/* 操作系统选择 */
#define OPERATE_SYSTEM      SYSTEM_NO
#define SYSTEM_NO           0
#define SYSTEM_FREERTOS     1

/* 内存堆定义 */
typedef struct MemHeapRegion
{
	uint8_t *pucStartAddress;
	size_t xSizeInBytes;
} MemHeapRegion_t;

typedef void (*MALLOC_FAIL_CB)(size_t xWantedSize);

typedef struct mem_manage_s
{
	MALLOC_FAIL_CB malloc_fail_cb;  // 内存申请失败时的回调，一般做重启系统处理
} mem_manage_t;

/************************************
 * @brief: 		初始化内存管理功能
 * @param[in] 	mem_manage
 * @return {*}  0-成功，其他失败
 *************************************/
int memManageFunctionInit(mem_manage_t *mem_manage, const MemHeapRegion_t * const pxHeapRegions);

/************************************
 * @brief: 		申请分配一块可用内存
 * @param[in] 	xWantedSize, 申请的内存块大小,单位字节
 * @return 		申请成功时，返回内存块的起始地址，失败返回空指针
 *************************************/
void *memMalloc( size_t xWantedSize );

/************************************
 * @brief: 		申请释放一块内存
 * @param[in] 	之前申请的内存块地址
 * @return 		void
 *************************************/
void memFree( void *pv );

/************************************
 * @brief: 获取剩余可用内存块总空间，单位字节
 * @param[in] void
 * @return 单位字节
 *************************************/
size_t memGetFreeHeapSize( void );

/************************************
 * @brief: 获取空闲链表中内存块总数
 * @param[in] void
 * @return 
 *************************************/
size_t memGetFreeBlockNum( void );

/************************************
 * @brief: 打印空闲链表内存块大小分布情况
 * @param[in] void
 * @return 
 *************************************/
void memPrintfFreeListLayout(void);

#if 0
#define MEM_MALLOC		malloc
#define MEM_FREE		free
#else
#define MEM_MALLOC		memMalloc
#define MEM_FREE		memFree
#endif

#ifndef configASSERT
#define configASSERT( x )
#endif

// 字节对齐宏定义
#if memBYTE_ALIGNMENT == 8
	#define memBYTE_ALIGNMENT_MASK ( 0x0007U )
#endif

#if memBYTE_ALIGNMENT == 4
	#define memBYTE_ALIGNMENT_MASK	( 0x0003U)
#endif

#ifdef __cplusplus
}
#endif

#endif
