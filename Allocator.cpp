#include "Allocator.h"
#include "DataTypes.h"
#include <new>
#include <assert.h>

//------------------------------------------------------------------------------
// Constructor

/* Initsializiruyem razmer bloka: yesli razmer bloka men'she, chem razmer ukazatelya (sizeof(long*)), 
to ustanavlivayem minimal'nyy razmer bloka ravnym razmeru ukazatelya, inache ispol'zuyem zadannyy razmer.

Ustanavlivayem ob"yekt m_name (imya allokatora).

V zavisimosti ot togo, peredan li parametr memory, vybirayem rezhim raboty allokatora:
STATIC_POOL: yesli pamyat' peredana vneshne.
HEAP_POOL: yesli my sozdayem pul iz kuchi (s kolichestvom ob"yektov).
HEAP_BLOCKS: yesli ne zadano kolichestvo ob"yektov, sozdayem novyy blok po mere neobkhodimosti. */

//------------------------------------------------------------------------------

// razm bloca | kolvo object (in heap, not obazatelno) | ykazatel on vydelennyu memory (if peredali) | name allocator for statistic
Allocator::Allocator(size_t size, UINT objects, CHAR* memory, const CHAR* name) :
    m_blockSize(size < sizeof(long*) ? sizeof(long*):size),								// min raxm bloca = razm ykazatela
    m_objectSize(size),
    m_maxObjects(objects),
    m_pHead(NULL),
    m_poolIndex(0),
    m_blockCnt(0),
    m_blocksInUse(0),
    m_allocations(0),
    m_deallocations(0),
    m_name(name)
{
    // If using a fixed memory pool 
	if (m_maxObjects)
	{
		// If caller provided an external memory pool
		if (memory)
		{
			m_pPool = memory;
			m_allocatorMode = STATIC_POOL;
		}
		else 
		{
			m_pPool = (CHAR*)new CHAR[m_blockSize * m_maxObjects];
			m_allocatorMode = HEAP_POOL;									// pool from heap
		}
	}
	else
		m_allocatorMode = HEAP_BLOCKS;										// heap mode
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
Allocator::~Allocator()
{
	// If using pool then destroy it, otherwise traverse free-list and 
	// destroy each individual block
	if (m_allocatorMode == HEAP_POOL)
		delete [] m_pPool;
	else if (m_allocatorMode == HEAP_BLOCKS)									// if heap, delete all blocks
	{
		while(m_pHead)
			delete [] (CHAR*)Pop();										// delete all from free-list
	}
}

//------------------------------------------------------------------------------
// Allocate
//------------------------------------------------------------------------------
void* Allocator::Allocate(size_t size)
{
    assert(size <= m_objectSize);											// chek razmer_a
	
    // If can't obtain existing block then get a new one
    void* pBlock = Pop();												 // if no free blocks:
    if (!pBlock)
    {
        // If using a pool method then get block from pool,
        // otherwise using dynamic so get block from heap
        if (m_maxObjects)
        {
            // If we have not exceeded the pool maximum
            if(m_poolIndex < m_maxObjects)
            {
                pBlock = (void*)(m_pPool + (m_poolIndex++ * m_blockSize));
            }
            else
            {
                // Get the pointer to the new handler
                std::new_handler handler = std::set_new_handler(0);							// obrabotchik error_ov
                std::set_new_handler(handler);

                // If a new handler is defined, call it
                if (handler)
                    (*handler)();
                else
                    assert(0);												// break();
            }
        }
        else
        {
        	m_blockCnt++;
            pBlock = (void*)new CHAR[m_blockSize];
        }
    }

    m_blocksInUse++;													 // schetchiki
    m_allocations++;
	
    return pBlock;													 // ykazatel
}

//------------------------------------------------------------------------------
// Deallocate														// vozvrat in free-list
//------------------------------------------------------------------------------
void Allocator::Deallocate(void* pBlock)
{
    Push(pBlock);
	m_blocksInUse--;
	m_deallocations++;
}

//------------------------------------------------------------------------------
// Push															// go block to free-list
//------------------------------------------------------------------------------
void Allocator::Push(void* pMemory)
{
    Block* pBlock = (Block*)pMemory;											// privedenie ykazatela
    pBlock->pNext = m_pHead;
    m_pHead = pBlock;													 // a new head of heap   0_o
}

//------------------------------------------------------------------------------
// Pop															
//------------------------------------------------------------------------------
void* Allocator::Pop()
{
    Block* pBlock = NULL;

    if (m_pHead)
    {
        pBlock = m_pHead;
        m_pHead = m_pHead->pNext;
    }

    return (void*)pBlock;
}





