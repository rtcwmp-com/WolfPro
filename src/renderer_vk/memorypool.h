#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H
//#pragma once

#include "tr_local.h"



typedef struct {
	uint32_t nextFreeIndex;
	uint16_t generation;
	uint16_t inUse;
} memoryPoolItem;

typedef struct {
	uint16_t poolType;
	uint32_t firstFree;
	uint32_t typeSize;
	uint32_t itemCount;
	uint8_t *poolData;
	memoryPoolItem *lookupData;
} memoryPool;

typedef struct {
	uint32_t index;
	uint16_t generation;
	uint16_t type;
} DecomposedHandle;

typedef struct PoolIterator {
	int index;
	void *value;
	uint64_t handle;
} PoolIterator;

void Pool_Clear(memoryPool* pool);
void Pool_Init(memoryPool *pool, const uint32_t itemCount, const uint32_t typeSize, uint16_t type);
uint64_t ComposeHandle(uint32_t index, uint16_t generation,uint16_t type);
DecomposedHandle DecomposeHandle(uint64_t handle);
uint64_t Pool_Add(memoryPool *pool, void *rawItem);
void Pool_Remove(memoryPool *pool, uint64_t handle);
void* Pool_Get(memoryPool *pool, uint64_t handle);
PoolIterator Pool_BeginIteration(void);
qboolean Pool_Iterate(memoryPool *pool, PoolIterator *it);
uint64_t Pool_Size(memoryPool *pool);


#endif