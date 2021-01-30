
#ifndef __MEM_MANAGE_H__
#define __MEM_MANAGE_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "SEGGER_RTT.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define memBYTE_ALIGNMENT   8
#define MEM_NO_HANDLE( x )
#define MEM_MANAGE_PRINTF(fmt, ...)     SEGGER_RTT_printf(0,fmt, ##__VA_ARGS__)

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

void *pvPortMalloc( size_t xWantedSize );
void vPortDefineHeapRegions( const MemHeapRegion_t * const pxHeapRegions );
void vPortFree( void *pv );

void xMemDebugPrintfFreeBlock(void);

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
