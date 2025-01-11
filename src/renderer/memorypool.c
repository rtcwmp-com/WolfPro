#include "memorypool.h"


void Pool_Clear(memoryPool* pool) {
	pool->firstFree = 1;
	for (int i = 1; i < pool->itemCount; i++) {
		pool->lookupData[i].inUse = 0;
		pool->lookupData[i].nextFreeIndex = i < (pool->itemCount - 1) ? i + 1 : 0;
		pool->lookupData[i].generation = 0;
	}
}

void Pool_Init(memoryPool *pool, const uint32_t itemCount, const uint32_t typeSize, uint16_t type){
	pool->poolType = type;
	pool->typeSize = typeSize;
	pool->itemCount = itemCount;
	pool->poolData = (uint8_t*)malloc((itemCount + 1) * typeSize);
	pool->lookupData = (memoryPoolItem*)malloc((itemCount + 1) * sizeof(memoryPoolItem));
	Pool_Clear(pool);
}

uint64_t ComposeHandle(uint32_t index, uint16_t generation,uint16_t type){
	return ((uint64_t)index << 32) | ((uint64_t)generation << 16) | (uint64_t)type;
}


DecomposedHandle DecomposeHandle(uint64_t handle){
	DecomposedHandle item;
	item.index = (handle >> 32) & 0xFFFFFFFF;
	item.generation = (handle >> 16) & 0xFFFF;
	item.type = (handle) & 0xFFFF;
	return item;
}

uint64_t Pool_Add(memoryPool *pool, void *rawItem){
	uint32_t freeSpot = pool->firstFree;
	
	if(freeSpot == 0){
		ri.Error(ERR_FATAL, "(Pool_Add)Ran out of pool memory\n");	
	}	

	uint8_t *poolLocation = pool->poolData + (freeSpot * pool->typeSize);
	memcpy(poolLocation, rawItem, pool->typeSize);
	pool->lookupData[freeSpot].inUse = qtrue;
	pool->lookupData[freeSpot].generation++;
	pool->firstFree = pool->lookupData[freeSpot].nextFreeIndex;
	
	return ComposeHandle(freeSpot, pool->lookupData[freeSpot].generation, pool->poolType);
}

static void HandleChecks(DecomposedHandle item, memoryPool* pool, const char* operation){
	if(item.type != pool->poolType){
		ri.Error(ERR_FATAL, "(%s)Wrong pool type\n", operation);
	}

	if(item.index >= pool->itemCount){
		ri.Error(ERR_FATAL, "(%s)Invalid index into pool\n", operation);
	}

	if(item.generation < pool->lookupData[item.index].generation){
		ri.Error(ERR_FATAL, "(%s)Invalid handle generation (too old)\n", operation);
	}

	if(item.generation > pool->lookupData[item.index].generation){
		ri.Error(ERR_FATAL, "(%s)Invalid handle generation (too new)\n", operation);
	}

	if(pool->lookupData[item.index].inUse == qfalse){
		ri.Error(ERR_FATAL, "(%s)Invalid handle index (not in use)\n", operation);
	}
}

void Pool_Remove(memoryPool *pool, uint64_t handle){
	DecomposedHandle item = DecomposeHandle(handle);
	HandleChecks(item, pool, "Pool_Remove");
	pool->lookupData[item.index].nextFreeIndex = pool->firstFree;
	pool->firstFree = item.index;
}

void* Pool_Get(memoryPool *pool, uint64_t handle){
	DecomposedHandle item = DecomposeHandle(handle);
	HandleChecks(item, pool, "Pool_Get");

	uint32_t itemLocation = item.index * pool->typeSize;
	return pool->poolData+itemLocation;
}