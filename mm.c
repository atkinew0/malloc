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
#include "defs.h"




/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

//#define checkheap() mm_heapcheck()
#define checkheap()

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

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


static void place(void *bp, size_t asize);
static void *extend_heap(size_t words);
void *mm_malloc(size_t size);
static void* find_fit(size_t asize);
void mm_free(void *ptr);
static void *coalesce(void *bp);
void *mm_realloc(void *ptr, size_t size);
static void mm_heapcheck();


//GLOBAL VARIABLES
void * heap_listp;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    printf("Starting mm_init\n");
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;

    PUT(heap_listp,0);                                        //Alignment padding
    PUT(heap_listp + (1* WSIZE), PACK(DSIZE, ALLOCATED));     //Prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, ALLOCATED));    //Prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, ALLOCATED));        //Epilogue header
    heap_listp += (2*WSIZE);

    checkheap();

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL ){
        return -1;
    }
    printf("Completed mm_init, heap_listp now at %p \n", heap_listp);
    return 0;

}

static void *extend_heap(size_t words){


    char* bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if((long)(bp = mem_sbrk(size)) == 1)
        return NULL;

    bp += 16; //bump up bp because of next and prev linked list pointers

    
    //Initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, UNALLOCATED));   //new free header
    PUT(FTRP(bp), PACK(size, UNALLOCATED));   //new free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, ALLOCATED));  //new eiplogue block

    checkheap();

    return coalesce(bp); 

}

static void mm_heapcheck(){

    void* start = heap_listp-2;
    int count = 0;

    while(1){

        printf("%d (%p): %d ", count++, start, (*(int *) start));
        printf("Size at ptr: %d Alloc at ptr %d \n", GET_SIZE(start), GET_ALLOC(start));

        start += 1;

        if( GET_SIZE(start) == 0 && GET_ALLOC(start) == 1){
            printf("saw epilogue so break\n");
            break;
        }
        

    }



}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    printf("Call to mm_malloc\n");
    printf("Heap size currently: %ld \n", mem_heapsize());

    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0)
        return NULL;
    if(size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);

    return bp;



    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
}

static void *find_fit(size_t asize){

    void* bp = heap_listp;
    printf("in FIND_FIT try to find block for %ld\n", asize);


    while(GET_SIZE(HDRP(bp)) > 0){
        printf("size of this current block in find_fit is %d %d\n", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }

        bp = NEXT_BLKP(bp);
        
    }
    printf("WHILE loop finished find fit returning NULL - we couldn't find a fit\n");
    return NULL;
    
    // void* bp;
    // for( bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp) ) {

    //     if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
    //         return bp;
    //     }

    // }
    // return NULL;

}

static void place(void *bp, size_t asize){

    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (2* DSIZE)){

        PUT(HDRP(bp), PACK(asize, ALLOCATED));   //resize header on current block were allocating
        PUT(FTRP(bp), PACK(asize, ALLOCATED));   //resize footer on current block were allocating
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize -asize, UNALLOCATED));  //create new block after this one with remaining space
        PUT(FTRP(bp), PACK(csize - asize, UNALLOCATED));  //create new block after this one with remaining space


    }else{
        PUT(HDRP(bp), PACK(csize, ALLOCATED));        //dont change size, just mark to allocated
        PUT(FTRP(bp), PACK(csize, ALLOCATED));        //dont change size just mark to allocated the footer
    }



}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    printf("Call to mm_free\n");
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, UNALLOCATED));
    PUT(FTRP(bp), PACK(size, UNALLOCATED));
    coalesce(bp);

}

static void *coalesce(void *bp){

    printf("Coalescing...");

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));


    printf("Prev and next alloc are %ld %ld\n", prev_alloc, next_alloc);

    if(prev_alloc && next_alloc){
        printf("cannot coalesce\n");
        return bp;
    }

    else if(prev_alloc && !next_alloc){
        printf("coalesce with next block\n");
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, UNALLOCATED));
        PUT(FTRP(bp), PACK(size, UNALLOCATED));
    }

    else if (!prev_alloc && next_alloc){
        printf("coalesce with prev block\n");
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, UNALLOCATED));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, UNALLOCATED));
        bp = PREV_BLKP(bp);
    }

    else {
        printf("coalesce with both prev and next blocks\n");
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, UNALLOCATED));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, UNALLOCATED));
        bp = PREV_BLKP(bp);



    }

    return bp;


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
