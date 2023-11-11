/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Liu Zhaocheng",
    /* First member's full name */
    "BlackGhost",
    /* First member's email address */
    "2427324519@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

void *heap_list;
void *coalesce(void *bp);

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = val)
#define GET_SIZE(p) (GET(p) & (~0x7))
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HEAD(p) (p - WSIZE)
#define FOOT(p) (p + GET_SIZE(p) - DSIZE)
#define PACK(size, alloc) (size | alloc)

// 这里返回的是块的有效部分， 传进去的也是块的第一个有效字
#define NEXT_BLOCK(p) (p + GET_SIZE(p - WSIZE))
#define PREV_BLOCK(p) (p - GET_SIZE(p - DSIZE))
void *extend_heap(size_t words)
{
    void *bp;
    size_t size = 0;
    // 需要保证8个字节的对齐，即两个字, 传进来的参数是字
    if (words % 2)
    {
        size = (words + 1) * WSIZE;
    }
    else
    {
        size = words * WSIZE;
    }
    // sbrk需要的参数是字节
    bp = mem_sbrk(size);
    if ((long)bp == -1)
        return NULL;

    // 填充首和尾
    PUT(bp - WSIZE, PACK(size, 0));
    PUT(FOOT(bp), PACK(size, 0));
    PUT(NEXT_BLOCK(bp), PACK(0, 1));

    // 合并
    return coalesce(bp);
}

int mm_init(void)
{
    heap_list = mem_sbrk(4 * WSIZE);
    if (heap_list == (void *)(-1))
        return -1;
    // 大致结构 :   填充字(4) 序言块(8) 填充(4)
    PUT(heap_list, 0);
    PUT(heap_list + WSIZE, PACK(DSIZE, 1));
    PUT(heap_list + 2 * WSIZE, PACK(DSIZE, 1));
    PUT(heap_list + 3 * WSIZE, PACK(0, 1));
    heap_list += 2 * WSIZE;
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HEAD(ptr));

    PUT(HEAD(ptr), PACK(size, 0));
    PUT(FOOT(ptr), PACK(size, 0));

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FOOT(PREV_BLOCK(bp))); /* 前一块大小 */
    size_t next_alloc = GET_ALLOC(HEAD(NEXT_BLOCK(bp))); /* 后一块大小 */
    size_t size = GET_SIZE(HEAD(bp));                    /* 当前块大小 */

    /*
     * 四种情况：前后都不空, 前不空后空, 前空后不空, 前后都空
     */
    /* 前后都不空 */
    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    /* 前不空后空 */
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HEAD(NEXT_BLOCK(bp))); // 增加当前块大小
        PUT(HEAD(bp), PACK(size, 0));           // 先修改头
        PUT(FOOT(bp), PACK(size, 0));           // 根据头部中的大小来定位尾部
    }
    /* 前空后不空 */
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HEAD(PREV_BLOCK(bp))); // 增加当前块大小
        PUT(FOOT(bp), PACK(size, 0));
        PUT(HEAD(PREV_BLOCK(bp)), PACK(size, 0));
        bp = PREV_BLOCK(bp); // 注意指针要变
    }
    /* 都空 */
    else
    {
        size += GET_SIZE(HEAD(PREV_BLOCK(bp))) + GET_SIZE(FOOT(NEXT_BLOCK(bp))); // 增加当前块大小
        PUT(FOOT(NEXT_BLOCK(bp)), PACK(size, 0));
        PUT(HEAD(PREV_BLOCK(bp)), PACK(size, 0));
        bp = PREV_BLOCK(bp);
    }
    return bp;
}
