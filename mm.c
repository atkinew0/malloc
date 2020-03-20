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
#include <assert.h>

#include "mm.h"
#include "memlib.h"
#include "defs.h"




/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define checkheap() mm_heapcheck()
//#define checkheap()

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
static void splice_out(void * bp);


//GLOBAL VARIABLES
void * heap_listp;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    printf("\n\n========================Call to mm_init===================\n");

    if((heap_listp = mem_sbrk(12*WSIZE)) == (void*)-1)
        return -1;

    printf("Heapstart at %p\n", heap_listp);

    PUT(heap_listp, 0);                                         //padding
    PUT(heap_listp + (1 * WSIZE), PACK(24, ALLOCATED));          //prologue header
    PUT_P(heap_listp + (2 * WSIZE) , 0);                              //prolopgue prev points to 0 NULL
    PUT_P(heap_listp + (4 * WSIZE), heap_listp + 32);                         //prologue next set to be epilogue
    PUT(heap_listp + (6 * WSIZE), PACK(24, ALLOCATED));      //prologue footer
    PUT(heap_listp + (7 * WSIZE), PACK(0, ALLOCATED));          //epilogue header
    PUT_P(heap_listp + ( 8 * WSIZE), heap_listp + (2 * WSIZE));                //epilogue prev set to prologue header
    PUT_P(heap_listp + (10* WSIZE), 0);                       //epilogue next set to 0 NULL
    heap_listp += ( 2 * WSIZE);                            //skip padding to point heap_listp at prologue

    printf("Heapstart at %p\n", heap_listp);
    printf("Get size works on heap start ptr? %d \n", GET_SIZE(HDRP(heap_listp)));
   

    printf("HERE in mm_init print the heap so far to check-\n\n");
    printf("%p \n", GET(heap_listp - (2 * WSIZE)));
    printf("%p \n", GET(heap_listp - (1*WSIZE)));
    printf("%p \n", GET_P(heap_listp  ));
    printf("%p \n", GET_P(heap_listp + (2 * WSIZE)));
    printf("%p \n", GET(heap_listp + (4 * WSIZE)));  //prologue footer
    printf("%p \n", GET(heap_listp + (5 * WSIZE)));  //epilogue header
    printf("%p \n", GET_P(heap_listp + (6 * WSIZE)));  //epilogue prev
    printf("%p \n", GET_P(heap_listp + (8 * WSIZE)));  //epilogue next


    if(extend_heap(CHUNKSIZE/WSIZE) == NULL ){
        //return -1;
    }
    printf("Completed mm_init, heap_listp now at %p \n", heap_listp);

    return 0;


}

static void *extend_heap(size_t words){

    printf("\n\n========================Call to extend_heap===================\n");
    printf("\n\nExtend heap by n: %d \n words", words);

    char* bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if((long)(bp = mem_sbrk(size)) == 1)
        return NULL;

    void ** address_old_prev = bp - 16;
    bp -= 16;
   
    printf("Address of start of new free area %p\n",bp);
    printf("For ep_last we saved %lx\n",*address_old_prev);

    PUT(HDRP(bp), PACK(size, UNALLOCATED));
    PUT(FTRP(bp), PACK(size, UNALLOCATED));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, ALLOCATED));

    printf("Correctly wrote HDRP? %d\n", GET_SIZE(HDRP(bp)));
    printf("Correctly wrote FTRP? %d\n", GET_SIZE(FTRP(bp)));
    printf("Correctly wrote size of next blkp %d\n", GET_SIZE(HDRP(NEXT_BLKP(bp))));
    printf("Correttly wrote alloc of next blkp %d\n",GET_ALLOC(HDRP(NEXT_BLKP(bp))));
    printf("Address of next blkp calculated at %lx \n", NEXT_BLKP(bp));
    printf("------------------Address calc for prev_blkp is %p \n", PREV_BLKP(bp));

    //now wire up the pointers
    PUT_P(bp, *address_old_prev);   //set the new free blocks prev pointer
    //printf("SEt the new free block's prev pointer to old prev from epilogue %lx\n", old_prev);

    PUT_P(bp + 8, NEXT_BLKP(bp) );      //set the new free blocks next pointer
    printf("This says the location of the NEXT-BLKP is %p\n", NEXT_BLKP(bp));
    printf("New free blocks prev pointer now points to %p\n", PREV_LL(bp));
    printf("New free blocks next pointer now points to %p\n", NEXT_LL(bp));
    printf("At that next pointer we have size %d\n", GET_SIZE(HDRP(NEXT_LL(bp))));



    PUT_P(NEXT_BLKP(bp) + 8, 0); //set the newly written epilogue section's  next to point to 0, null
    PUT_P(NEXT_BLKP(bp), bp ); //set the newly written epilogue section's prev to point to the new free block
    PUT_P(PREV_BLKP(bp) + 8, bp);       //set the old prev's next to point to the new free block

    printf("1 Now the newly written epilogues prev points to %ld\n", PREV_LL(NEXT_BLKP(bp)));
    printf("2 Now the old prev's next points to %ld\n", NEXT_LL(PREV_BLKP(bp)));

    
    //Initialize free block header/footer and the epilogue header
    // PUT(HDRP(bp), PACK(size, UNALLOCATED));   //new free header
    // PUT(FTRP(bp), PACK(size, UNALLOCATED));   //new free block footer
    // PUT(HDRP(NEXT_BLKP(bp)), PACK(0, ALLOCATED));  //new eiplogue block

    // checkheap();

    return coalesce(bp); 

}

static void mm_heapcheck(){

    printf("\n                _-_-_-_-_-_-_-_-_-_-_-_-_ HEAPCHECK TIME _-_-_-_-_-_-_-_-_-_-_-_-_-\n");
    //1 Check the freelist to see its reasonable size and not allocated
    void* start = heap_listp;
    int free_count = 0;

    if( GET_SIZE(HDRP(start)) != 24 || GET_ALLOC(HDRP(start)) != 1){
        printf("Incorrectly configured prologue, exiting!!\n");
        exit(1);
    }else{
        printf("Prologue OK size 24 alloc 1\n");
    }

    start = NEXT_LL(start);

    while(GET_SIZE(HDRP(start)) > 0){

        if(free_count > 100){
            printf("Seems checkloop abort \n");
            exit(1);
        }

        printf("Checking freelist entry %d: size : %d allocated %d\n", free_count++, GET_SIZE(HDRP(start)), GET_ALLOC(HDRP(start)));
        if(GET_ALLOC(HDRP(start)) != 0){
            printf("Detected incorectly allocated freelist entry\n");
            exit(1);
        }
        start = NEXT_LL(start);
    }

    if( GET_SIZE(HDRP(start)) == 0 && GET_ALLOC(HDRP(start)) != 1){
        printf("Epilogue block err detected!!\n");
        exit(1);
    }else{
        printf("Epilogue OK size 0 alloc 1\n");
    }

    printf("+++++Check 1 OK Counted %d correctly unallocated entries in the freelist+++++\n", free_count);

    //2 heapcheck 2 -traverse the free list making sure all entries in the linked list are mutually pointing at each other!

    start = heap_listp;
    

    while(1){

        //make sure my NEXT_LL is correctly pointing BACK at me
        

        if(GET_SIZE(HDRP(start)) == 0){
            printf("Size zero must be epilogue so break\n");
            break;
        }

        void * next_prev = PREV_LL(NEXT_LL(start));
        assert(start == next_prev);
        printf("OK -- at %p size %d points back at %p ", start, GET_SIZE(HDRP(start)), PREV_LL(start));
        
        
        printf("About to shift to next-ll at %p\n", NEXT_LL(start));
        start = NEXT_LL(start);
    }

    printf("+++++Check 2 OK all next pointers correctly point back\n");


}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    printf("\n\n========================Call to MM_MALLOC===================\n");
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
    printf("\nin FIND_FIT try to find block for %ld\n", asize);
    int attempt = 0;

    while(GET_SIZE(HDRP(bp)) > 0){
        printf("size of this current block in find_fit is %d %d\n", GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            printf("Found fit!\n");
            return bp;
        }

        bp = NEXT_LL(bp);
        printf("Here in find_fit about to jump from %p to %p\n", bp, NEXT_LL(bp));
        attempt++;
        if(attempt > 50 ) break;
        
    }
    printf("WHILE loop finished find fit returning NULL - we couldn't find a fit\n");
    return NULL;
    

}

static void place(void *bp, size_t asize){

    printf("PLACE a block of size %ld, starting at location %p\n", asize, bp);
    size_t csize = GET_SIZE(HDRP(bp));

    if((csize - asize) >= (3* DSIZE)){

        printf("Fragmenting new free block of %ld\n", (csize-asize));
        printf("HERE in the 6088 block (%p) MY prev in the ll IS %p\n", bp, PREV_LL(bp));

        PUT(HDRP(bp), PACK(asize, ALLOCATED));   //resize header on current block were allocating
        PUT(FTRP(bp), PACK(asize, ALLOCATED));   //resize footer on current block were allocating
        bp = NEXT_BLKP(bp);
        printf("Now jumped to next %p\n", bp);

        PUT(HDRP(bp), PACK(csize -asize, UNALLOCATED));  //create new block after this one with remaining space
        PUT(FTRP(bp), PACK(csize - asize, UNALLOCATED));  //create new block after this one with remaining space
        printf("HERE in the 6088 block (%p) MY prev in the ll IS %p\n", bp, PREV_LL(PREV_BLKP(bp)));


        //adjust pointers to wire in the new free where the alloced is
        //bp should be currently at new free

        printf("From newly fragment previous block LL pREV is %p size %d \n", PREV_LL(PREV_BLKP(bp)), GET_SIZE(HDRP(PREV_LL(PREV_BLKP(bp)))));
        printf("From newly fragmented just regular prev block is %p \n", PREV_BLKP(bp));
        printf("bp is at %p, prologue is at %p\n", bp, heap_listp);

        PUT_P(PREV_LL(PREV_BLKP(bp))+ 8, bp);   //adjust alloceds prev's next to new free
        PUT_P(bp, PREV_LL(PREV_BLKP(bp)));      //write a new prev to new free - copy alloceds
        PUT_P(bp + 8, NEXT_LL(PREV_BLKP(bp)) ); //write a new next to the new free - copy alloceds
        PUT_P(NEXT_LL(bp), bp);                 //Adjust alloced's next's prev to new free
        printf("From newly fragment previous block LL pREV is %p size %d \n", PREV_LL(PREV_BLKP(bp)), GET_SIZE(HDRP(PREV_LL(PREV_BLKP(bp)))));

    
    }else{
        PUT(HDRP(bp), PACK(csize, ALLOCATED));        //dont change size, just mark to allocated
        PUT(FTRP(bp), PACK(csize, ALLOCATED));        //dont change size just mark to allocated the footer
        splice_out(bp);
    }



}

//Adds a newly free block (freed or split) to the front of the free list just after prologue
void splice_out(void * bp){

    printf("Doing a splice OUT");
    PUT_P(PREV_LL(bp) + 8, NEXT_LL(bp));     //point the newly allocated blocks prev's next to allocated blocks next
    PUT_P(NEXT_LL(bp), PREV_LL(bp));         //point the allocated blocks next's prev to allocated blocks prev

    // printf("Spliceout test- allocated's prev points to allocated's next? ");
    // printf("PREV's NEXT %p\n", (void *) GET_P((PREV_LL(bp) + 8)) );
    // printf("CUrrent's next's prev points to? %p\n", (void *) GET_P(NEXT_LL(bp)));
    assert(GET_P(PREV_LL(bp) + 8) == NEXT_LL(bp));

}

//Adding a newly free block to front of list by wiring it after prologue
void splice_in(void * bp){

    void * saved_next =  GET_P(heap_listp + 8);  //save what prologue's old next was
    PUT_P(heap_listp + 8, bp);                //now point prologues next to new free block
    PUT_P(bp, heap_listp);                    //set new free blocks prev to prologue
    PUT_P(bp + 8, saved_next );               //set new free blocks next to saved next
    PUT_P(saved_next, bp);                    //finally set the next blocks prev to new free block

    printf("SPLICE IN test- bp %p and its next %p\n",bp, (void *) NEXT_LL(bp) );

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    printf("\n\n========================Call to mm_free===================\n");
    size_t size = GET_SIZE(HDRP(bp));
    printf("Freeing a block of size %d\n", size);

    PUT(HDRP(bp), PACK(size, UNALLOCATED));
    PUT(FTRP(bp), PACK(size, UNALLOCATED));
    splice_in(bp);
    checkheap();
    coalesce(bp);

    checkheap();

}

static void *coalesce(void *bp){

    printf("\n\n----------------------------Coalescing------------------------------------...\n");

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));



    if(prev_alloc && next_alloc){
        printf("cannot coalesce\n");
        return bp;
    }

    else if(prev_alloc && !next_alloc){
        printf("coalesce with next block\n");
        

        // PUT_P(PREV_LL(NEXT_BLKP(bp)) + 8, bp );     //point the prev/prologue at newly freed block
        // printf("The next BLKP at %p size %d and its next LL (epilogue) at %p\n", NEXT_BLKP(bp), GET_SIZE(HDRP(NEXT_BLKP(bp))), NEXT_LL(NEXT_BLKP(bp)));
        // PUT_P(NEXT_LL(NEXT_BLKP(bp)), bp);          //point the next's next or epilogue back at me
        // PUT_P(bp, PREV_LL(NEXT_BLKP(bp)));          //newly freed blocks prev is next frees prev
        // printf("PREV for the next blkp is %p \n", PREV_LL(NEXT_BLKP(bp)));
        // PUT_P(bp + 8, NEXT_LL(NEXT_BLKP(bp)) );     //newly freed blocks prev is old frees prev

        PUT_P(bp + 8, NEXT_LL(NEXT_BLKP(bp)));          //write new free base a new next pointer to skip over nextblkp
        PUT_P(NEXT_LL(NEXT_BLKP(bp)), bp );                //go all the way past nextblkp to its next and write its prev to here
        

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, UNALLOCATED));
        PUT(FTRP(bp), PACK(size, UNALLOCATED));



    }

    else if (!prev_alloc && next_alloc){

        printf("coalesce with prev block\n");
        printf("I am %p and my prev is %p ok\n",bp, PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        PUT_P(PREV_BLKP(bp)+8, NEXT_LL(bp));        //make prev blocks next my next to wire me out
        PUT_P(NEXT_LL(bp), PREV_BLKP(bp));          //make my next point around me to my prev

        
        printf("new size up of %d yeah boii\n",size);
        PUT(FTRP(bp), PACK(size, UNALLOCATED));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, UNALLOCATED));

        
        
        
    }

    else {
        printf("coalesce with both prev and next blocks\n");
        
        PUT_P(PREV_BLKP(bp) + 8, NEXT_LL(NEXT_BLKP(bp)));
        PUT_P(NEXT_LL(NEXT_BLKP(bp)), PREV_BLKP(bp) );

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, UNALLOCATED));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, UNALLOCATED));
        bp = PREV_BLKP(bp);



    }
    printf("END of coalesce new big free at %p and its prev at %p\n", bp, PREV_LL(bp));
    checkheap();
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
