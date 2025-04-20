/*
File > Open Folder
(Ctrl+Shift+P) → CMake: Select a Kit
CMake: Configure
CMake: Build
CMake: Run
*/

# Copyright (c) 2022 David Lafreniere  
# MIT License  
# Этот файл содержит модификации, внесенные в 2025 году.
# https://github.com/endurodave/Allocator


#include "Allocator.h"
#include <assert.h>
#include <new>
#include <iostream>
#include <chrono>
#include <cstdlib> 

// @see https://github.com/endurodave/Allocator

// On VisualStudio, to disable the debug heap for faster performance when using
// the debugger use this option:
// Debugging > Environment _NO_DEBUG_HEAP=1

class MyClass 
{
	DECLARE_ALLOCATOR
	// remaining class definition
};
IMPLEMENT_ALLOCATOR(MyClass, 0, 0)

// Heap blocks mode unlimited with 100 byte blocks
Allocator allocatorHeapBlocks(100);

// Heap pool mode with 20, 100 byte blocks
Allocator allocatorHeapPool(100, 20);

// Static pool mode with 20, 100 byte blocks
char staticMemoryPool[100 * 20];
Allocator allocatorStaticPool(100, 20, staticMemoryPool);

// Static pool mode with 20 MyClass sized blocks using template
AllocatorPool<MyClass, 20> allocatorStaticPool2;

// Benchmark allocators
static const int MAX_BLOCKS = 10000;
static const int MAX_BLOCK_SIZE = 4096;
void* memoryPtrs[MAX_BLOCKS];
void* memoryPtrs2[MAX_BLOCKS];
AllocatorPool<char[MAX_BLOCK_SIZE], MAX_BLOCKS*2> allocatorStaticPoolBenchmark;
Allocator allocatorHeapBlocksBenchmark(MAX_BLOCK_SIZE);

static void out_of_memory()
{
	// new-handler function called by Allocator when pool is out of memory
	assert(0);
}

typedef void* (*AllocFunc)(int size);
typedef void (*DeallocFunc)(void* ptr);
void Benchmark(const char* name, AllocFunc allocFunc, DeallocFunc deallocFunc, int f);
void BenchmarkDefault(const char* name, int f);
void* AllocHeap(int size);
void DeallocHeap(void* ptr);
void* AllocStaticPool(int size);
void DeallocStaticPool(void* ptr);
void* AllocHeapBlocks(int size);
void DeallocHeapBlocks(void* ptr);

//------------------------------------------------------------------------------
// main

/*
MyClass* obj1 = new MyClass();       
MyClass* obj2 = ::new MyClass();      
*/

//------------------------------------------------------------------------------

int main(void)
{
	std::set_new_handler(out_of_memory);

	MyClass* myClass = new MyClass();
	delete myClass;

	void* memory1 = allocatorHeapBlocks.Allocate(100);
	allocatorHeapBlocks.Deallocate(memory1);

	void* memory2 = allocatorHeapBlocks.Allocate(100);
	allocatorHeapBlocks.Deallocate(memory2);

	void* memory3 = allocatorHeapPool.Allocate(100);
	allocatorHeapPool.Deallocate(memory3);

	void* memory4 = allocatorStaticPool.Allocate(100);
	allocatorStaticPool.Deallocate(memory4);

	void* memory5 = allocatorStaticPool2.Allocate(sizeof(MyClass));
	allocatorStaticPool2.Deallocate(memory5);

//	0-> only TOTAL TIME, 1-> all info
int flag = 0; 
	
	BenchmarkAllocator("Heap (Run 1)", AllocHeap, DeallocHeap, flag);
BenchmarkDefault("Standard new/delete (Run 1)", flag);
	BenchmarkAllocator("Heap (Run 2)", AllocHeap, DeallocHeap, flag);
BenchmarkDefault("Standard new/delete (Run 2)", flag);
	BenchmarkAllocator("Heap (Run 3)", AllocHeap, DeallocHeap, flag);
BenchmarkDefault("Standard new/delete (Run 3)", flag);

	BenchmarkAllocator("Static Pool (Run 1)", AllocStaticPool, DeallocStaticPool, flag);
BenchmarkDefault("Standard new/delete (Run 1)", flag);
	BenchmarkAllocator("Static Pool (Run 2)", AllocStaticPool, DeallocStaticPool, flag);
BenchmarkDefault("Standard new/delete (Run 2)", flag);
	BenchmarkAllocator("Static Pool (Run 3)", AllocStaticPool, DeallocStaticPool, flag);
BenchmarkDefault("Standard new/delete (Run 3)", flag);

	BenchmarkAllocator("Heap Blocks (Run 1)", AllocHeapBlocks, DeallocHeapBlocks, flag);
BenchmarkDefault("Standard new/delete (Run 1)", flag);
	BenchmarkAllocator("Heap Blocks (Run 2)", AllocHeapBlocks, DeallocHeapBlocks, flag);
BenchmarkDefault("Standard new/delete (Run 2)", flag);
	BenchmarkAllocator("Heap Blocks (Run 3)", AllocHeapBlocks, DeallocHeapBlocks, flag);
BenchmarkDefault("Standard new/delete (Run 3)", flag);
	
	return 0;
}

//------------------------------------------------------------------------------
// AllocHeap
//------------------------------------------------------------------------------
void* AllocHeap(int size)
{
	return new CHAR[size];
}

//------------------------------------------------------------------------------
// DeallocHeap
//------------------------------------------------------------------------------
void DeallocHeap(void* ptr)
{
	delete [] ptr;
}

//------------------------------------------------------------------------------
// AllocStaticPool
//------------------------------------------------------------------------------
void* AllocStaticPool(int size)
{
	return allocatorStaticPoolBenchmark.Allocate(size);
}

//------------------------------------------------------------------------------
// DeallocStaticPool
//------------------------------------------------------------------------------
void DeallocStaticPool(void* ptr)
{
	allocatorStaticPoolBenchmark.Deallocate(ptr);
}

//------------------------------------------------------------------------------
// AllocHeapBlocks
//------------------------------------------------------------------------------
void* AllocHeapBlocks(int size)
{
	return allocatorHeapBlocksBenchmark.Allocate(size);
}

//------------------------------------------------------------------------------
// DeallocHeapBlocks
//------------------------------------------------------------------------------
void DeallocHeapBlocks(void* ptr)
{
	allocatorHeapBlocksBenchmark.Deallocate(ptr);
}

//------------------------------------------------------------------------------
// Benchmark
//------------------------------------------------------------------------------
void Benchmark(const char* name, AllocFunc allocFunc, DeallocFunc deallocFunc, int f)
{
#if WIN32
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds, TotalElapsedMicroseconds= {0};
	LARGE_INTEGER Frequency;

	SetProcessPriorityBoost(GetCurrentProcess(), true);

	QueryPerformanceFrequency(&Frequency); 

	// Allocate MAX_BLOCKS blocks MAX_BLOCK_SIZE / 2 sized blocks
	QueryPerformanceCounter(&StartingTime);
	for (int i=0; i<MAX_BLOCKS; i++)
		memoryPtrs[i] = allocFunc(MAX_BLOCK_SIZE / 2);
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " allocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	// Deallocate MAX_BLOCKS blocks (every other one)
	QueryPerformanceCounter(&StartingTime);
	for (int i=0; i<MAX_BLOCKS; i+=2)
		deallocFunc(memoryPtrs[i]);
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	// Allocate MAX_BLOCKS blocks MAX_BLOCK_SIZE sized blocks
	QueryPerformanceCounter(&StartingTime);
	for (int i=0; i<MAX_BLOCKS; i++)
		memoryPtrs2[i] = allocFunc(MAX_BLOCK_SIZE);
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " allocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	// Deallocate MAX_BLOCKS blocks (every other one)
	QueryPerformanceCounter(&StartingTime);
	for (int i=1; i<MAX_BLOCKS; i+=2)
		deallocFunc(memoryPtrs[i]);
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	// Deallocate MAX_BLOCKS blocks 
	QueryPerformanceCounter(&StartingTime);
	for (int i=MAX_BLOCKS-1; i>=0; i--)
		deallocFunc(memoryPtrs2[i]);
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	std::cout << name << " TOTAL TIME: " << TotalElapsedMicroseconds.QuadPart << std::endl;

	SetProcessPriorityBoost(GetCurrentProcess(), false);
#endif
}

void BenchmarkDefault(const char* name, int f)
{
#if WIN32
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds, TotalElapsedMicroseconds = { 0 };
	LARGE_INTEGER Frequency;

	void* memoryPtrs[MAX_BLOCKS];
	void* memoryPtrs2[MAX_BLOCKS];

	QueryPerformanceFrequency(&Frequency);
	if(f){
	std::cout << "Running: " << name << std::endl;
	}
	// Allocate MAX_BLOCKS blocks (2048 байт)
	QueryPerformanceCounter(&StartingTime);
	for (int i = 0; i < MAX_BLOCKS; i++)
		memoryPtrs[i] = ::new char[MAX_BLOCK_SIZE / 2];
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " allocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	// Deallocate половину
	QueryPerformanceCounter(&StartingTime);
	for (int i = 0; i < MAX_BLOCKS; i += 2)
		::delete[] static_cast<char*>(memoryPtrs[i]);
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	// Allocate снова, но по 4096 байт
	QueryPerformanceCounter(&StartingTime);
	for (int i = 0; i < MAX_BLOCKS; i++)
		memoryPtrs2[i] = ::new char[MAX_BLOCK_SIZE];
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " allocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	// Освобождаем вторую половину
	QueryPerformanceCounter(&StartingTime);
	for (int i = 1; i < MAX_BLOCKS; i += 2)
		::delete[] static_cast<char*>(memoryPtrs[i]);
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	// Освобождаем последние
	QueryPerformanceCounter(&StartingTime);
	for (int i = MAX_BLOCKS - 1; i >= 0; i--)
		::delete[] static_cast<char*>(memoryPtrs2[i]);
	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	if(f){
	std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
	}
	TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

	std::cout << name << " TOTAL TIME: " << TotalElapsedMicroseconds.QuadPart << std::endl;
#endif
}



