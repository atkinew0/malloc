
#define WSIZE 4 //word size
#define DSIZE 8 //data size          
#define CHUNKSIZE (1<<12) //4K.
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc)) //flag that represents both block size & if_allocated.
#define GET(p) (*(unsigned int *)(p)) //load address.
#define PUT(p,val) (*(unsigned int *)(p) = (val)) // save address.
#define PUT_P(p,val) (*(unsigned long *)(p) = (unsigned long *)(val))
#define GET_P(p) (*(unsigned long *)(p))
#define GET_SIZE(p) (GET(p) & ~0x7) //get size.
#define GET_ALLOC(p) (GET(p) & 0x1) //get if_allocated.
#define HDRP(bp) ((char *)(bp) - WSIZE) // get header pointer.
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)// get footer pointer.
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //next block pointer.
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //previous block pointer looks to prev ftr for size calc.
#define NEXT_LL(bp) (*(intptr_t *)((char *)(bp) + DSIZE))
#define PREV_LL(bp) (*(intptr_t *)(bp))
#define HDRP_FREE(bp)  ((char *)(bp) - (WSIZE +  (2 * DSIZE) ))
#define GET_NEXT(p) (GET( (char *)(p) +12  ))
#define GET_PREV(p) (GET( (char *)(p) + 4 ))
#define ALLOCATED 1
#define UNALLOCATED 0