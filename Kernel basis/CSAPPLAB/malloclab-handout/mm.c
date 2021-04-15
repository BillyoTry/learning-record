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
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define WSIZE    4
#define DSIZE    8
#define CHUNKSIZE (1<<12)     /*Extend heap by this amount(bytes)*/
#define MAX(x,y)  ((x) > (y)?(x):(y))

/* Pack a size and allocated bit into word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)  (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p*/
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
static char* heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp,size_t asize);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if((heap_listp=mem_sbrk(4*WSIZE))==(void*)-1)
        return -1;
    PUT(heap_listp,0);                              //alignment padding
    PUT(heap_listp+(1*WSIZE),PACK(DSIZE,1));//prologue header
    PUT(heap_listp+(2*WSIZE),PACK(DSIZE,1));//prologue footer
    heap_listp+=(2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;        //adjusted block size
    size_t extendsize;   //amount to extend heap if no fit
    char *bp;

    /* Ignore spurious requests */
    if(size==0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs*/
    if(size<=DSIZE)
        asize=2*DSIZE;
    else
        asize=DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);

    /* Search the free list for a fit*/
    if((bp=find_fit(asize))!=NULL){
        place(bp,asize);  //空闲块分配asize大小
        return bp;
    }

    /* No fit found. Get more memory and place the block*/
    extendsize=MAX(asize,CHUNKSIZE);
    if((bp=extend_heap(extendsize/WSIZE))==NULL)
        return NULL;
    place(bp,asize);
    return bp;
}
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size=(words%2)?(words+1)*WSIZE:words*WSIZE;
    if((long)(bp=mem_sbrk(size))==-1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp),PACK(size,0));     //free block header
    PUT(HDRP(bp),PACK(size,0));     //free block footer
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1)); //new epilogue header

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/*立即合并*/
static void *coalesce(void *bp)
{
    /*虽然传入的是当前块，但是会检查前块和后块的已分配/空闲位来合并*/
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size=GET_SIZE(HDRP(bp));

    /*前后块都已分配*/
    if(prev_alloc&&next_alloc){
        return bp; //无法合并，返回当前块
    }
    /*前块分配，后块空闲*/
    else if(prev_alloc&&!next_alloc){
        size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));//与后块合并
        PUT(HDRP(bp),PACK(size,0));  //将大小写进当前块的头部
        PUT(FTRP(bp),PACK(size,0));  //将大小写进当前块的脚部
    }
    /*前块空闲，后块已分配*/
    else if(!prev_alloc&&next_alloc){
        size+=GET_SIZE(HDRP(PREV_BLKP(bp)));//与前块合并
        PUT(FTRP(bp),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        bp=PREV_BLKP(bp);
    }
    /*前后块都空闲*/
    else if(!prev_alloc&&!next_alloc){
        size+=GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        bp=PREV_BLKP(bp);
    }
    return bp;
}

static void *find_fit(size_t asize)
{
    /* First-fit search */
    void *bp;
    for(bp=heap_listp;GET_SIZE(HDRP(bp))>0;bp=NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp))&&(asize<=GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL; /* No fit */
}

static void place(void *bp,size_t asize)
{
    size_t csize=GET_SIZE(HDRP(bp));
    if(csize-asize>=2*DSIZE){
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        bp=NEXT_BLKP(bp);
        PUT(HDRP(bp),PACK(csize-asize,0));
        PUT(FTRP(bp),PACK(csize-asize,0));
    }else{
        PUT(HDRP(bp),PACK(csize,1));
        PUT(FTRP(bp),PACK(csize,1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size=GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(ptr=NULL){ //if ptr is NULL, the call is equivalent to mm malloc(size);
        return mm_malloc(size);
    }
    if(size==0){ //if size is equal to zero, the call is equivalent to mm free(ptr);
        mm_free(ptr);
        return NULL;
    }

    size_t asize; //对齐后的值
    if(size<=DSIZE) asize=2*DSIZE;
    else asize=DSIZE*((size+DSIZE+(DSIZE-1))/DSIZE);

    size_t oldsize=GET_SIZE(HDRP(ptr));
    if(oldsize==asize) return ptr;
    else if (oldsize>asize){
        PUT(HDRP(ptr),PACK(asize,1));
        PUT(FTRP(ptr),PACK(asize,1));
        PUT(HDRP(NEXT_BLKP(ptr)),PACK(oldsize-asize,0)); //多于的部分变为空闲
        PUT(FTRP(NEXT_BLKP(ptr)),PACK(oldsize-asize,0));
        return ptr;
    }else{
        size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        if(!next_alloc&&GET_SIZE(HDRP(NEXT_BLKP(ptr)))+oldsize>=asize){
            size_t last=GET_SIZE(HDRP(NEXT_BLKP(ptr)))+oldsize-asize;
            PUT(HDRP(ptr),PACK(asize,1));
            PUT(FTRP(ptr),PACK(asize,1));
            if(last>=DSIZE){
                PUT(HDRP(NEXT_BLKP(ptr)),PACK(last,0));
                PUT(FTRP(NEXT_BLKP(ptr)),PACK(last,0));
            }
            return ptr;
        }else{
            char *newptr=mm_malloc(asize);
            if(newptr==NULL) return NULL;
            memcpy(newptr,ptr,oldsize-DSIZE);
            mm_free(ptr);
            return newptr;
        }
    }
}














