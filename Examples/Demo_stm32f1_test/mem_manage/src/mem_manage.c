/**
 * @file: mem_manage.c
 * @author: LinusZhao
 * @brief: 嵌入式裸机或RTOS系统下内存管理的实现
 * @version: 1.0.0
 * @date: 2021-01-01
 * @attention: 基本算法思想来源于 FreeRTOS heap5
 **/

#include "mem_manage.h"

#if !defined(MEM_MANAGE_PRINTF)
	#error "please define MEM_MANAGE_PRINTF Macro, in mem_manage.h file !!!"
#endif

/*-----------------------------------------------------------*/

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( memBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) memBYTE_ALIGNMENT_MASK );

/* 允许出现的最小尺寸内存块. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

// 空闲内存块计数，表征内存碎片化情况
static size_t xFreeBlockNum = 0;

mem_manage_t xMemManage;

/*-----------------------------------------------------------*/

void *memMalloc( size_t xWantedSize )
{
	BlockLink_t *pxBlock, *pxBlock_used, *pxPreviousBlock, *pxPreviousBlock_used, *pxNewBlockLink;
	void *pvReturn = NULL;

	// size_t search_depth = 0;  // 尝试查找更优内存块的深度

	/* The heap must be initialised before the first call to
	prvPortMalloc(). */
	if (pxEnd == NULL){
		return NULL;
	}

#if defined(OPERATE_SYSTEM) && (OPERATE_SYSTEM == SYSTEM_FREERTOS)
	vTaskSuspendAll();
#elif defined(OPERATE_SYSTEM) && (OPERATE_SYSTEM == SYSTEM_NO)
#else
    #error "please define OPERATE_SYSTEM Macro !!!"
#endif
	{
		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & memBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( memBYTE_ALIGNMENT - ( xWantedSize & memBYTE_ALIGNMENT_MASK ) );
				}
				else
				{
					MEM_NO_HANDLE(0);
				}
			}
			else
			{
                MEM_NO_HANDLE(0); 
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if( pxBlock != pxEnd )
				{
					pxBlock_used = pxBlock;
					pxPreviousBlock_used = pxPreviousBlock;
				#if defined(TATTER_OPTIME_EN) && (TATTER_OPTIME_EN > 0)
					// 找到了第一个可用内存块，且很大，需要分隔，真的需要分隔，不再找找
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE ){
						pxBlock_used = pxBlock;  // 保存可用的pxBlock
						pxPreviousBlock_used = pxPreviousBlock;
						// 找下一个更合适的内存块
						pxPreviousBlock = pxBlock;
						pxBlock = pxBlock->pxNextFreeBlock;
						while(pxBlock->pxNextFreeBlock != NULL){
							if((pxBlock->xBlockSize >= xWantedSize) \
								&& ((pxBlock->xBlockSize - xWantedSize) <= heapMINIMUM_BLOCK_SIZE)){
								// 找到新的更优内存块
								pxBlock_used = pxBlock;
								pxPreviousBlock_used = pxPreviousBlock;
							}

							#if 0   // 为优化效率考虑的，碎片较多时，查询可能比较耗时，可控制查询深度
							search_depth++;
							if(search_depth >= 10)
								break; // 不找了，
							#endif

							// 下一个内存块
							pxPreviousBlock = pxBlock;
							pxBlock = pxBlock->pxNextFreeBlock;
						}
					}
				#endif
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock_used->pxNextFreeBlock ) + xHeapStructSize );

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock_used->pxNextFreeBlock = pxBlock_used->pxNextFreeBlock;
					xFreeBlockNum--;
					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock_used->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock_used ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock_used->xBlockSize - xWantedSize;
						pxBlock_used->xBlockSize = xWantedSize;
						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					}
					else
					{
                        MEM_NO_HANDLE(0); 
					}

					xFreeBytesRemaining -= pxBlock_used->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
                        MEM_NO_HANDLE(0);
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock_used->xBlockSize |= xBlockAllocatedBit;
					pxBlock_used->pxNextFreeBlock = NULL;
				}
				else
				{
                    MEM_NO_HANDLE(0); 
				}
			}
			else
			{
                MEM_NO_HANDLE(0);
			}
		}
		else
		{
            MEM_NO_HANDLE(0); 
		}
	}
#if defined(OPERATE_SYSTEM) && (OPERATE_SYSTEM == SYSTEM_FREERTOS)
	( void ) xTaskResumeAll();
#elif defined(OPERATE_SYSTEM) && (OPERATE_SYSTEM == SYSTEM_NO)
#else
    #error "please define OPERATE_SYSTEM Macro !!!"
#endif
	if( pvReturn == NULL )
	{
		if (xMemManage.malloc_fail_cb)
			xMemManage.malloc_fail_cb(xWantedSize);
	}
	else
	{
		MEM_NO_HANDLE(0);
	}

	return pvReturn;
}
/*-----------------------------------------------------------*/

void memFree( void *pv )
{
	uint8_t *puc = ( uint8_t * ) pv;
	BlockLink_t *pxLink;

	if( pv != NULL )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		configASSERT( pxLink->pxNextFreeBlock == NULL );

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

            #if defined(OPERATE_SYSTEM) && (OPERATE_SYSTEM == SYSTEM_FREERTOS)
	            vTaskSuspendAll();
            #elif defined(OPERATE_SYSTEM) && (OPERATE_SYSTEM == SYSTEM_NO)
            #else
                #error "please define OPERATE_SYSTEM Macro !!!"
            #endif
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}
            #if defined(OPERATE_SYSTEM) && (OPERATE_SYSTEM == SYSTEM_FREERTOS)
	            ( void ) xTaskResumeAll();
            #elif defined(OPERATE_SYSTEM) && (OPERATE_SYSTEM == SYSTEM_NO)
            #else
                #error "please define OPERATE_SYSTEM Macro !!!"
            #endif

			}
			else
			{
				MEM_NO_HANDLE(0);
			}
		}
		else
		{
			MEM_NO_HANDLE(0);
		}
	}
}
/*-----------------------------------------------------------*/

size_t memGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t memGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}

size_t memGetFreeBlockNum( void )
{
	return xFreeBlockNum;
}

// {"xMemFreeListLayout":[12,12],"num":2}
void memPrintfFreeListLayout(void)
{
	BlockLink_t *pxIterator = &xStart;
	size_t num = 0,freeBlockTotalSize = 0;

	MEM_MANAGE_PRINTF("\n{\"xMemFreeListLayout\":[");
	while(pxIterator->pxNextFreeBlock != NULL)
	{
		if(pxIterator->xBlockSize > 0) {
			// MEM_MANAGE_PRINTF("{\"size\":%ld,\"0x\"%08x},",pxIterator->xBlockSize,(size_t)(pxIterator));
			MEM_MANAGE_PRINTF("%ld,",pxIterator->xBlockSize);
			freeBlockTotalSize += pxIterator->xBlockSize;
			num++;
		}
		pxIterator = pxIterator->pxNextFreeBlock;
	}
	MEM_MANAGE_PRINTF("%ld],\"num\":%d}\n",freeBlockTotalSize,num);
}

/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
    BlockLink_t *pxIterator;
    uint8_t *puc;

	xFreeBlockNum++;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	// 按地址升序的方向插入空闲内存块
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	// 左边连续地址的空闲内存块，可以合并
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
		xFreeBlockNum--;
	}
	else
	{
		MEM_NO_HANDLE(0);
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
			xFreeBlockNum--;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
		MEM_NO_HANDLE(0);
	}
}
/*-----------------------------------------------------------*/

static void memDefineHeapRegions( const MemHeapRegion_t * const pxHeapRegions )
{
    BlockLink_t *pxFirstFreeBlockInRegion = NULL, *pxPreviousFreeBlock;
    size_t xAlignedHeap;
    size_t xTotalRegionSize, xTotalHeapSize = 0;
    size_t xDefinedRegions = 0;
    size_t xAddress;
    const MemHeapRegion_t *pxHeapRegion;

	/* Can only call once! */
	configASSERT( pxEnd == NULL );

	pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );

	while( pxHeapRegion->xSizeInBytes > 0 )
	{
		xTotalRegionSize = pxHeapRegion->xSizeInBytes;

		/* Ensure the heap region starts on a correctly aligned boundary. */
		xAddress = ( size_t ) pxHeapRegion->pucStartAddress;
		if( ( xAddress & memBYTE_ALIGNMENT_MASK ) != 0 )
		{
			xAddress += ( memBYTE_ALIGNMENT - 1 );
			xAddress &= ~memBYTE_ALIGNMENT_MASK;

			/* Adjust the size for the bytes lost to alignment. */
			xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;
		}

		xAlignedHeap = xAddress;

		/* Set xStart if it has not already been set. */
		if( xDefinedRegions == 0 )
		{
			/* xStart is used to hold a pointer to the first item in the list of
			free blocks.  The void cast is used to prevent compiler warnings. */
			xStart.pxNextFreeBlock = ( BlockLink_t * ) xAlignedHeap;
			xStart.xBlockSize = ( size_t ) 0;
		}
		else
		{
			/* Should only get here if one region has already been added to the
			heap. */
			configASSERT( pxEnd != NULL );

			/* Check blocks are passed in with increasing start addresses. */
			configASSERT( xAddress > ( size_t ) pxEnd );
		}

		/* Remember the location of the end marker in the previous region, if
		any. */
		pxPreviousFreeBlock = pxEnd;

		/* pxEnd is used to mark the end of the list of free blocks and is
		inserted at the end of the region space. */
		xAddress = xAlignedHeap + xTotalRegionSize;
		xAddress -= xHeapStructSize;
		xAddress &= ~memBYTE_ALIGNMENT_MASK;
		pxEnd = ( BlockLink_t * ) xAddress;
		pxEnd->xBlockSize = 0;
		pxEnd->pxNextFreeBlock = NULL;

		/* To start with there is a single free block in this region that is
		sized to take up the entire heap region minus the space taken by the
		free block structure. */
		pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
		pxFirstFreeBlockInRegion->xBlockSize = xAddress - ( size_t ) pxFirstFreeBlockInRegion;
		pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd;

		/* If this is not the first region that makes up the entire heap space
		then link the previous region to this region. */
		if( pxPreviousFreeBlock != NULL )
		{
			pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
		}

		xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

		xFreeBlockNum++;
		/* Move onto the next MemHeapRegion_t structure. */
		xDefinedRegions++;
		pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
	}

	xMinimumEverFreeBytesRemaining = xTotalHeapSize;
	xFreeBytesRemaining = xTotalHeapSize;

	/* Check something was actually defined before it is accessed. */
	configASSERT( xTotalHeapSize );

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );

	memPrintfFreeListLayout();
}

int memManageFunctionInit(mem_manage_t *mem_manage, const MemHeapRegion_t * const pxHeapRegions)
{
	if( mem_manage == NULL )
		return -1;

	memcpy(&xMemManage,mem_manage,sizeof(mem_manage_t));
	memDefineHeapRegions(pxHeapRegions);
	return 0;
}

// void* pvPortReAlloc( void *pv,  size_t xWantedSize )
// {
// 	BlockLink_t *pxLink;
// 	unsigned char *puc = ( unsigned char * ) pv;

// 	if( pv )
// 	{
// 		if( !xWantedSize )
// 		{
// 			memFree( pv );
// 			return NULL;
// 		}

// 		void *newArea = memMalloc( xWantedSize );
// 		if( newArea )
// 		{
// 			/* The memory being freed will have an xBlockLink structure immediately
// 				before it. */
// 			puc -= xHeapStructSize;

// 			/* This casting is to keep the compiler from issuing warnings. */
// 			pxLink = ( void * ) puc;

// 			int oldSize =  (pxLink->xBlockSize & ~xBlockAllocatedBit) - xHeapStructSize;
// 			int copySize = ( oldSize < xWantedSize ) ? oldSize : xWantedSize;
// 			memcpy( newArea, pv, copySize );

// 			vTaskSuspendAll();
// 			{
// 				/* Add this block to the list of free blocks. */
// 				pxLink->xBlockSize &= ~xBlockAllocatedBit;
// 				xFreeBytesRemaining += pxLink->xBlockSize;
// 				prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
// 			}
// 			xTaskResumeAll();
// 			return newArea;
// 		}
// 	}
// 	else if( xWantedSize )
// 		return memMalloc( xWantedSize );
// 	else
// 		return NULL;

// 	return NULL;
// }

void *pvPortCalloc(size_t xWantedCnt, size_t xWantedSize)
{
	void *p;

	/* allocate 'xWantedCnt' objects of size 'xWantedSize' */
	p = memMalloc(xWantedCnt * xWantedSize);
	if (p) {
		/* zero the memory */
		memset(p, 0, xWantedCnt * xWantedSize);
	}
	return p;
}




