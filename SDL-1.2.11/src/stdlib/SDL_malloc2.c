
#include "SDL_config.h"

/* This file contains portable memory management functions for SDL */

#include "SDL_stdinc.h"


#ifdef USE_BLOCK_MALLOC

#include "GPlayerPorting.h"

// for SWF Malloc
#include "dlmalloc.h"

void* gSDL_BlockMemory = NULL;

void* gSDL_Start_BlockMemory = NULL;
void* gSDL_End_BlockMemory = NULL;


// for porting
extern void* gTarget_BlockMemAddress;
extern unsigned int gTarget_MpegMemSize;


//#define	CHECK_BLOCK_MEM_SIZE


int USE_SystemMemoryBlock_Init(void)
{
	if (gSDL_BlockMemory != NULL)
	{
		printf("[SDL] Already create_mspace_with_base() \n");
		return 0;
	}
	
	if (gTarget_BlockMemAddress == NULL)
	{
		printf("[SDL] System Block Memory Allocation fail !!!\n");
		return 0;
	}

	printf("[%s] Allocated Block Memory Base Addr : 0x%x [%d]MByte -#%dline\n", 
					__FUNCTION__, gTarget_BlockMemAddress, gTarget_MpegMemSize/(1024*1024), __LINE__);

	gSDL_BlockMemory = (void*)dlCreateAllocator(gTarget_BlockMemAddress, gTarget_MpegMemSize);


	/* Set Block Malloc Marker */
	gSDL_Start_BlockMemory = gSDL_BlockMemory;
	gSDL_End_BlockMemory = gSDL_BlockMemory;

	printf("[%s] UserBlockMemory : 0x%x \n", __FUNCTION__, gSDL_BlockMemory);
	printf("[%s] Temp. Modification for Game Hang : jijoo 2008.08.03 \n", __FUNCTION__ );

	return 1;
}


int USE_SystemMemoryBlock_End(void)
{
	printf("[%s] Video Memory USE End \n", __FUNCTION__);

	if (gSDL_BlockMemory == NULL)
	{
		printf("Have No mspace or already destroied msapce!!\n");
		return 0;
	}

	dlDestroyAllocator(gSDL_BlockMemory);
	gSDL_BlockMemory = NULL;


	/* Set Block Malloc Marker */
	gSDL_Start_BlockMemory = NULL ;
	gSDL_End_BlockMemory = NULL ;

	printf("[%s] End USE_SystemMemoryBlock_End() \n", __FUNCTION__);

	return 1;
}



void * SDL_malloc(size_t size)
{

	void *mspace_alloc_mem = NULL;

	//if ((gSDL_BlockMemory != NULL) && (size >= 256))
	if ((gSDL_BlockMemory != NULL) && (size >= 1024))
	{
		mspace_alloc_mem = dlmalloc(gSDL_BlockMemory, (unsigned int)size) ;

		if (mspace_alloc_mem != NULL)
		{
#ifdef CHECK_BLOCK_MEM_SIZE
			printf("======================================\n");
			printf("Call SDL_malloc >> SWF dlmalloc [0x%x] -- %d bytes\n ", mspace_alloc_mem, size);
			printf(" Available Memory : %d Bytes\n", dlGetAvailableMemory(gSDL_BlockMemory));
			printf("======================================\n\n");
#endif			

			if (mspace_alloc_mem > gSDL_End_BlockMemory)
			{
				gSDL_End_BlockMemory = mspace_alloc_mem ;
			}

			return mspace_alloc_mem ;
		}
	}

	return malloc(size);
}


void SDL_free(void *mem)
{
	if (mem == NULL)
	{
		printf("is NULL memory !!\n");
		return ;
	}

	if (gSDL_BlockMemory == NULL) 
	{
		printf("Have No mspace or already destroied msapce!!\n");
		return free(mem);
	}

//?????? =>> Maybe check Max dlmalloc address
#if 0
	if (mem < gSDL_BlockMemory)
	{
		printf("call free(0x%x) < gSDL_BlockMemory(0x%x)\n", mem, gSDL_BlockMemory);
		return free(mem);
	}

#else

	if ( (gSDL_Start_BlockMemory <= mem) && (mem <= gSDL_End_BlockMemory))
	{
#ifdef CHECK_BLOCK_MEM_SIZE
		printf("call gSDL_Start_BlockMemory(0x%x) <= dlfree(0x%x) <= gSDL_End_BlockMemory(0x%x)\n",  gSDL_Start_BlockMemory, mem, gSDL_End_BlockMemory);
#endif
		return dlfree(gSDL_BlockMemory, mem);
	}
	
#endif			
	
	return free(mem);
}


#endif	/*USE_BLOCK_MALLOC*/

