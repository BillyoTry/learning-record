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
//字大小和双字大小
#define WSIZE 4
#define DSIZE 8
//当堆内存不够时，向内核申请的堆空间
#define CHUNKSIZE (1<<12)
//将val放入p开始的4字节中
#define PUT(p,val) (*(unsigned int*)(p) = (val))
#define GET(p) (*(unsigned int*)(p))
//获得头部和脚部的编码
#define PACK(size, alloc) ((size) | (alloc))
//从头部或脚部获得块大小和已分配位
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLO(p) (GET(p) & 0x1)
//获得块的头部和脚部
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
//获得上一个块和下一个块
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

//获得块中记录后继和前驱的地址
#define PRED(bp) ((char*)(bp) + WSIZE)
#define SUCC(bp) ((char*)bp)
//获得块的后继和前驱的地址
#define PRED_BLKP(bp) (GET(PRED(bp)))
#define SUCC_BLKP(bp) (GET(SUCC(bp)))

#define MAX(x,y) ((x)>(y)?(x):(y))

static char *heap_listp;//heap_listp指向序言块的有效载荷
static char *listp;//listp指向大小类数组的起始位置
static void *extend_heap(size_t words);
static void *imme_coalesce(void *bp);
static void *first_fit(size_t asize);
static void place(void *bp,size_t asize);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(12*WSIZE)) == (void*)-1)
        return -1;
    //空闲块的最小块包含头部、前驱、后继和脚部，有16字节
    PUT(heap_listp+0*WSIZE, NULL);	//{16~31}
    PUT(heap_listp+1*WSIZE, NULL);	//{32~63}
    PUT(heap_listp+2*WSIZE, NULL);	//{64~127}
    PUT(heap_listp+3*WSIZE, NULL);	//{128~255}
    PUT(heap_listp+4*WSIZE, NULL);	//{256~511}
    PUT(heap_listp+5*WSIZE, NULL);	//{512~1023}
    PUT(heap_listp+6*WSIZE, NULL);	//{1024~2047}
    PUT(heap_listp+7*WSIZE, NULL);	//{2048~4095}
    PUT(heap_listp+8*WSIZE, NULL);	//{4096~inf}

    //还是要包含序言块和结尾块
    PUT(heap_listp+9*WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp+10*WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp+11*WSIZE, PACK(0, 1));//结尾块

    listp = heap_listp;
    heap_listp += 10*WSIZE;

    if(expend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}
static void *extend_heap(size_t words)
{
    size_t size;
    void *bp;

    size = words%2 ? (words+1)*WSIZE : words*WSIZE;	//对大小双字对对齐
    if((bp = mem_sbrk(size)) == (void*)-1)	//申请空间
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));	//设置头部
    PUT(FTRP(bp), PACK(size, 0));	//设置脚部
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	//设置新的结尾块

    PUT(PRED(bp), NULL);
    PUT(SUCC(bp), NULL);

    //立即合并
    bp = imme_coalesce(bp); //对该空闲块进行立即合并
    bp = add_block(bp); //将该空闲块插入合适的大小类的空闲链表中
    return bp;
}

/*立即合并*/
static void *imme_coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLO(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLO(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){
        return bp;
    }else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }else if(!prev_alloc && next_alloc){
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        delete_block(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }else{
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) +
                GET_SIZE(FTRP(PREV_BLKP(bp)));
        delete_block(NEXT_BLKP(bp));
        delete_block(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void delete_block(void *bp){ //从显示空闲链表中删除指定的空闲块
    PUT(SUCC(PRED_BLKP(bp)), SUCC_BLKP(bp));//调整前驱和后继的指针，使其跳过当前的空闲块
    if(SUCC_BLKP(bp)!=NULL)
        PUT(PRED(SUCC_BLKP(bp)), PRED_BLKP(bp));
}


static void *delay_coalesce(){
    void *bp = heap_listp;
    while(GET_SIZE(HDRP(bp)) != 0){
        if(!GET_ALLO(HDRP(bp)))
            bp = imme_coalesce(bp);
        bp = NEXT_BLKP(bp);
    }
}
/*在合并完空闲块后，我们需要将其插入到合适的大小类的显示空闲链表中*/
static void *add_block(void *bp){
    size_t size = GET_SIZE(HDRP(bp));
    int index = Index(size);
    void *root = listp+index*WSIZE;

    //LIFO
    return LIFO(bp, root);
    //AddressOrder
    //return AddressOrder(bp, root);
}
/*在将空闲块插入显示空闲链表时，首先需要确定该空闲块所在的大小类*/
static int Index(size_t size){
    int ind = 0;
    if(size >= 4096)
        return 8;

    size = size>>5;
    while(size){
        size = size>>1;
        ind++;
    }
    return ind;
}
//两种该显示空闲链表插入空闲块的策略
/*LIFO策略*/
static void *LIFO(void *bp, void *root){
    if(SUCC_BLKP(root)!=NULL){
        PUT(PRED(SUCC_BLKP(root)), bp);	//SUCC->BP
        PUT(SUCC(bp), SUCC_BLKP(root));	//BP->SUCC
    }else{
        PUT(SUCC(bp), NULL);
    }
    PUT(SUCC(root), bp);	//ROOT->BP
    PUT(PRED(bp), root);	//BP->ROOT
    return bp;
}
/*地址顺序策略*/
static void *AddressOrder(void *bp, void *root){
    void *succ = root;
    while(SUCC_BLKP(succ) != NULL){
        succ = SUCC_BLKP(succ);
        if(succ >= bp){
            break;
        }
    }
    if(succ == root){
        return LIFO(bp, root);
    }else if(SUCC_BLKP(succ) == NULL){
        PUT(SUCC(succ), bp);
        PUT(PRED(bp), succ);
        PUT(SUCC(bp), NULL);
    }else{
        PUT(SUCC(PRED_BLKP(succ)), bp);
        PUT(PRED(bp), PRED_BLKP(succ));
        PUT(SUCC(bp), succ);
        PUT(PRED(succ), bp);
    }
    return bp;
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    void *bp;

    if(size == 0)
        return NULL;
    //满足最小块要求和对齐要求，size是有效负载大小
    asize = size<=DSIZE ? 2*DSIZE : DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    //首次匹配
    if((bp = first_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
    //最佳匹配
    /*if((bp = best_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }*/
    if((bp = expend_heap(MAX(CHUNKSIZE, asize)/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

static void *first_fit(size_t asize)
{
    int ind = Index(asize);
    void *succ;
    while(ind <= 8){
        succ = listp+ind*WSIZE;
        while((succ = SUCC_BLKP(succ)) != NULL){
            if(GET_SIZE(HDRP(succ)) >= asize && !GET_ALLO(HDRP(succ))){
                return succ;
            }
        }
        ind+=1;
    }
    return NULL;
}

static void *best_fit(size_t asize){
    int ind = Index(asize);
    void *best = NULL;
    int min_size = 0, size;
    void *succ;
    while(ind <= 8){
        succ = listp+ind*WSIZE;
        while((succ = SUCC_BLKP(succ)) != NULL){
            size = GET_SIZE(HDRP(succ));
            if(size >= asize && !GET_ALLO(HDRP(succ)) && (size<min_size||min_size==0)){
                best = succ;
                min_size = size;
            }
        }
        if(best != NULL)
            return best;
        ind+=1;
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t remain_size;
    remain_size = GET_SIZE(HDRP(bp)) - asize;
    delete_block(bp);
    if(remain_size >= DSIZE*2){	//分割
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remain_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remain_size, 0));
        add_block(NEXT_BLKP(bp));
    }else{
        PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
        PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr){
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    //立即合并
    ptr = imme_coalesce(ptr);
    add_block(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size){
    size_t asize, ptr_size, remain_size;
    void *new_bp;

    if(ptr == NULL){
        return mm_malloc(size);
    }
    if(size == 0){
        mm_free(ptr);
        return NULL;
    }

    asize = size<=DSIZE ? 2*DSIZE : DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    new_bp = imme_coalesce(ptr);	//尝试是否有空闲的
    ptr_size = GET_SIZE(HDRP(new_bp));
    PUT(HDRP(new_bp), PACK(ptr_size, 1));
    PUT(FTRP(new_bp), PACK(ptr_size, 1));
    if(new_bp != ptr)
        memcpy(new_bp, ptr, GET_SIZE(HDRP(ptr)) - DSIZE);

    if(ptr_size == asize){
        return new_bp;
    }else if(ptr_size > asize){
        remain_size = ptr_size - asize;
        if(remain_size >= DSIZE*2){	//分割
            PUT(HDRP(new_bp), PACK(asize, 1));
            PUT(FTRP(new_bp), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(new_bp)), PACK(remain_size, 0));
            PUT(FTRP(NEXT_BLKP(new_bp)), PACK(remain_size, 0));
            add_block(NEXT_BLKP(new_bp));
        }
        return new_bp;
    }else{
        if((ptr = mm_malloc(asize)) == NULL)
            return NULL;
        memcpy(ptr, new_bp, ptr_size - DSIZE);
        mm_free(new_bp);
        return ptr;
    }
}