#include <stdio.h>


extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);
extern int mm_checkheap(void);


/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;




/* Basic constants and macros */
#define WSIZE 4   /* Word and header/footer size (bytes) */
#define DSIZE 8   /* Double word size (bytes)  */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x,y) ((x) > (y)? (x):(y))

/* Pack a size and allocated bit into a word */
#define PACK(size,alloc) ((size) | (alloc))


/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p)=(val))
#define GET_ADDRESS(p) (*(void **)(p))


//总size,包括头尾
/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)




/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp)+ GET_SIZE(HDRP(bp))-DSIZE)
#define PRED(bp) ((char *)(bp))       //祖先节点
#define SUCC(bp) ((char *)(bp)+WSIZE)   //后继结点
//#define SUCC(bp) ((char *)(bp))       //后继节点，只留后继结点
/* 获取有效字节，即获取总的size数-头尾指针(因为busyblock)没有前继和后继指针 */
#define GET_PAYLOAD(bp) (GET_SIZE(HDRP(bp))-DSIZE) 


/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))
