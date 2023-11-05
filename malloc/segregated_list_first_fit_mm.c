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

#define NEXT_FBLKP(bp) ((void *)*(unsigned *)(bp))
#define PREV_FBLKP(bp) ((void *)*((unsigned *)(bp) + 1))

#define SET_NEXT_FRBLK(bp, val) (*(unsigned *)(bp) = (unsigned)(val))
#define SET_PREV_FRBLK(bp, val) (*((unsigned *)(bp) + 1) = (unsigned)(val))

static char *heap_listp; // always points to the prologue block
static unsigned *block_list_start;

#define GET_IDX_FLIST_HEAD(idx) ((void *)(*(block_list_start + idx)))
#define SET_IDX_FLIST_HEAD(idx, val) (*(block_list_start + idx) = (unsigned)(val))

static size_t get_flist_idx(size_t size)
{
    int i = 0;
    if (size <= 32)
        i = 0;
    else if (size <= 64)
        i = 1;
    else if (size <= 128)
        i = 2;
    else if (size <= 256)
        i = 3;
    else if (size <= 512)
        i = 4;
    else if (size <= 1024)
        i = 5;
    else if (size <= 2048)
        i = 6;
    else if (size <= 4096)
        i = 7;
    else
        i = 8;
    return i;
}

static void remove_from_idx_flist(void *bp)
{
    void *prev = PREV_FBLKP(bp);
    void *next = NEXT_FBLKP(bp);

    SET_NEXT_FRBLK(bp, 0);
    SET_PREV_FRBLK(bp, 0);

    size_t idx = get_flist_idx(GET_SIZE(HDRP(bp)));
    /* case 1: free_listp == bp -> null */
    if (prev == NULL && next == NULL)
    {
        SET_IDX_FLIST_HEAD(idx, 0);
    }
    /* case 2: free_listp -> ... -> bp -> null */
    else if (prev != NULL && next == NULL)
    {
        SET_NEXT_FRBLK(prev, 0);
    }
    /* case 3: free_listp == bp -> ... */
    else if (prev == NULL && next != NULL)
    {
        SET_IDX_FLIST_HEAD(idx, next);
        SET_PREV_FRBLK(next, 0);
    }
    /* case 4: free_listp -> ... -> bp -> ... */
    else
    {
        SET_NEXT_FRBLK(prev, next);
        SET_PREV_FRBLK(next, prev);
    }
}

static void add_to_idx_flist(void *bp)
{
    size_t idx = get_flist_idx(GET_SIZE(HDRP(bp)));
    unsigned *idxlist = GET_IDX_FLIST_HEAD(idx);
    if (idxlist == NULL)
    {
        SET_IDX_FLIST_HEAD(idx, bp);
        SET_NEXT_FRBLK(bp, 0);
        SET_PREV_FRBLK(bp, 0);
        return;
    }
    SET_PREV_FRBLK(bp, 0);
    SET_NEXT_FRBLK(bp, idxlist);
    SET_PREV_FRBLK(idxlist, bp);
    SET_IDX_FLIST_HEAD(idx, bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t cur_size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
    }
    else if (prev_alloc && !next_alloc)
    {
        remove_from_idx_flist(NEXT_BLKP(bp));
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        cur_size += next_size;
        PUT(FTRP(NEXT_BLKP(bp)), PACK(cur_size, 0));
        PUT(HDRP(bp), PACK(cur_size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        remove_from_idx_flist(PREV_BLKP(bp));
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
        cur_size += prev_size;
        PUT(FTRP(bp), PACK(cur_size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(cur_size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        remove_from_idx_flist(PREV_BLKP(bp));
        remove_from_idx_flist(NEXT_BLKP(bp));
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(bp)));
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        cur_size += (prev_size + next_size);
        PUT(HDRP(PREV_BLKP(bp)), PACK(cur_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(cur_size, 0));
        bp = PREV_BLKP(bp);
    }
    add_to_idx_flist(bp);
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
    SET_PREV_FRBLK(bp, 0);
    SET_NEXT_FRBLK(bp, 0);

    add_to_idx_flist(bp);

    return bp;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(12 * WSIZE)) == ((void *)-1))
        return -1;

    block_list_start = (unsigned *)heap_listp;
    for (int i = 0; i < 9; ++i)
    {
        SET_IDX_FLIST_HEAD(i, 0);
    }
    PUT(block_list_start + 9, PACK(DSIZE, 1));
    PUT(block_list_start + 10, PACK(DSIZE, 1));
    PUT(block_list_start + 11, PACK(0, 1));
    heap_listp += (10 * WSIZE);
    if (extend_heap(CHUNK_SIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void place(void *bp, size_t asize)
{
    size_t cur_size = GET_SIZE(HDRP(bp));
    remove_from_idx_flist(bp);
    if (cur_size - asize >= (2 * DSIZE)) // need to Split
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        size_t remain = cur_size - asize;

        void *next_blkp = NEXT_BLKP(bp);
        PUT(HDRP(next_blkp), PACK(remain, 0));
        PUT(FTRP(next_blkp), PACK(remain, 0));
        coalesce(next_blkp);
    }
    else // not need to split
    {
        PUT(HDRP(bp), PACK(cur_size, 1));
        PUT(FTRP(bp), PACK(cur_size, 1));
    }
}

static void *first_fit(size_t size)
{
    size_t idx = get_flist_idx(size);
    while (idx < 9)
    {
        void *bp = GET_IDX_FLIST_HEAD(idx);
        while (bp != NULL)
        {
            size_t cur_size = GET_SIZE(HDRP(bp));
            if (cur_size >= size)
            {
                return bp;
            }
            bp = NEXT_FBLKP(bp);
        }
        idx++;
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

    void *bp = first_fit(size);
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
