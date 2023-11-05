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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    "",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & (~0x7))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNK_SIZE (1 << 12)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & (~0x7))
#define GET_ALLOC(p) (GET(p) & (0x1))

/*get its header and footer ptr of a given block ptr */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* get its next and prev block address */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

static char *heap_listp;
static char *prev_loc;

static void *coalesce(char *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t cur_size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        cur_size += next_size;
        PUT(FTRP(NEXT_BLKP(bp)), PACK(cur_size, 0));
        PUT(HDRP(bp), PACK(cur_size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
        cur_size += prev_size;
        PUT(FTRP(bp), PACK(cur_size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(cur_size, 0));
        if (prev_loc == bp)
        {
            bp = PREV_BLKP(bp);
            prev_loc = bp;
        }
        else
        {
            bp = PREV_BLKP(bp);
        }
    }
    else
    {
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        cur_size += (prev_size + next_size);
        PUT(HDRP(PREV_BLKP(bp)), PACK(cur_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(cur_size, 0));
        if (prev_loc == bp)
        {
            bp = PREV_BLKP(bp);
            prev_loc = bp;
        }
        else
        {
            bp = PREV_BLKP(bp);
        }
    }
    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size = (words & 0x1) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) // bp points to the location after the epilogue block
        return NULL;                       // bp also points to the new block's payload
    // 原来的结尾块变成了新的空闲块的头部,新的空闲块的最后一个字变成了新的结尾快
    PUT(HDRP(bp), PACK(size, 0));         // set new free block header
    PUT(FTRP(bp), PACK(size, 0));         // set new free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // set new epilogue block
    return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == ((void *)-1))
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + 2 * WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));
    heap_listp += 2 * WSIZE;
    if (extend_heap(CHUNK_SIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void place(void *bp, size_t asize)
{
    size_t cur_size = GET_SIZE(HDRP(bp));
    if (cur_size - asize >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        size_t remain = cur_size - asize;
        void *next_blkp = NEXT_BLKP(bp);
        PUT(HDRP(next_blkp), PACK(remain, 0));
        PUT(FTRP(next_blkp), PACK(remain, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(cur_size, 1));
        PUT(FTRP(bp), PACK(cur_size, 1));
    }
}

static void *find_next_fit(size_t size)
{
    char *bp = prev_loc;
    size_t cur_size = 0;
    if (bp != NULL)
    {
        cur_size = GET_SIZE(HDRP(bp));
    }
    while (bp && cur_size)
    {
        if (!GET_ALLOC(HDRP(bp)) && cur_size >= size)
        {
            prev_loc = bp;
            return bp;
        }
        bp = NEXT_BLKP(bp);
        cur_size = GET_SIZE(HDRP(bp));
    }

    bp = NEXT_BLKP(heap_listp);
    cur_size = GET_SIZE(HDRP(bp));
    while (bp != prev_loc && cur_size)
    {
        if (!GET_ALLOC(HDRP(bp)) && cur_size >= size)
        {
            prev_loc = bp;
            return bp;
        }
        bp = NEXT_BLKP(bp);
        cur_size = GET_SIZE(HDRP(bp));
    }
    return NULL;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;
    if (size <= DSIZE)
        size = 2 * DSIZE;
    else
    {
        size = ALIGN((size + DSIZE));
    }
    char *bp = find_next_fit(size);
    if (bp != NULL)
    {
        place(bp, size);
        return bp;
    }
    size_t extendsize = MAX(CHUNK_SIZE, size);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    // set this block to free
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // merge the former and the latter block (if exists)
    coalesce(bp);
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
    size = GET_SIZE(HDRP(newptr));
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize - DSIZE);
    mm_free(oldptr);
    return newptr;
}
