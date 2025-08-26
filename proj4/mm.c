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
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20231552",
    /* Your full name*/
    "JIMIN PARK",
    /* Your email address */
    "parkjm012@sogang.ac.kr",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE       4
#define DSIZE       8
#define CHUNKSIZE   (1<<9)  

#define MAX(x, y)   ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)   ((size) | (alloc))
#define GET(p)              (*(unsigned int *)(p))
#define PUT(p, val)         (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)         (GET(p) & ~0x7)
#define GET_ALLOC(p)        (GET(p) & 0x1)

#define HDRP(bp)            ((char *)(bp) - WSIZE)
#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* free block용 pred/succ 포인터 접근 매크로 */
#define PRED(bp)            (*(void **)(bp))
#define SUCC(bp)            (*(void **)((char *)(bp) + WSIZE))

#define CLASS_NUM 10 /*segreated class 수*/

/*             전역 변수 선언                           */

static char *heap_listp = NULL;
static void *seg_free_lists[CLASS_NUM];

/*      함수 선언            */
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void insert_free_block(void *bp);
static void remove_free_block(void *bp);
static int class_index(size_t size);

/*     디버깅용 체크 함수 */
int mm_check(void) {
    void *bp = heap_listp;
    while (GET_SIZE(HDRP(bp)) > 0) {
        if ((size_t)bp % DSIZE != 0) {
            printf("Error: block at %p is not aligned\n", bp);
            return 0;
        }
        if (GET(HDRP(bp)) != GET(FTRP(bp))) {
            printf("Error: header and footer do not match at %p\n", bp);
            return 0;
        }
        bp = NEXT_BLKP(bp);
    }

    for (int i = 0; i < CLASS_NUM; i++) {
        for (void *bp = seg_free_lists[i]; bp != NULL; bp = SUCC(bp)) {
            if (GET_ALLOC(HDRP(bp))) {
                printf("Error: allocated block %p in free list %d\n", bp, i);
                return 0;
            }
        }
    }

    return 1;
}





/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    for (int i = 0; i < CLASS_NUM; i++) seg_free_lists[i] = NULL;

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); /* padding */
    PUT(heap_listp + 1 * WSIZE, PACK(DSIZE, 1)); /* prologue header */
    PUT(heap_listp + 2 * WSIZE, PACK(DSIZE, 1)); /* prologue footer */
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));     /* epilogue header */
    heap_listp += 2 * WSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    //printf("[malloc] request %zu bytes\\n", size); //디버깅용
    size_t asize;
    size_t extendsize;
    void *bp;

    if (size == 0) return NULL;
    asize = (size <= DSIZE) ? 2*DSIZE : DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    
    if (size == DSIZE) {
    void *bp = find_fit(4 * DSIZE);
        if (bp) {
            place(bp, 2 * DSIZE);
            return bp;
        }
    }

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }


    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    //assert(mm_check()); 
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL) return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
    //assert(mm_check()); 
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL) return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t asize = (size <= DSIZE) ? 2 * DSIZE : ALIGN(size + DSIZE);
    // 축소하는 경우
    if (asize <= old_size) {
        // 분할할 수 있는지 확인
        if (old_size - asize >= 2 * DSIZE) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            
            void *remain = NEXT_BLKP(ptr);
            PUT(HDRP(remain), PACK(old_size - asize, 0));
            PUT(FTRP(remain), PACK(old_size - asize, 0));
            insert_free_block(remain);
        }
        return ptr;
    }

    void *next_bp = NEXT_BLKP(ptr);
    size_t next_alloc = GET_ALLOC(HDRP(next_bp));
    size_t next_size = GET_SIZE(HDRP(next_bp));
    size_t total = old_size + (next_alloc ? 0 : next_size);

    // 병합 가능한 경우: next block free
    if (!next_alloc&&total >= asize) {
            remove_free_block(next_bp);
            // 분할 여부 확인
            if (total - asize >= 2 * DSIZE) {
                PUT(HDRP(ptr), PACK(asize, 1));
                PUT(FTRP(ptr), PACK(asize, 1));

                void *remain = NEXT_BLKP(ptr);
                PUT(HDRP(remain), PACK(total - asize, 0));
                PUT(FTRP(remain), PACK(total - asize, 0));
                coalesce(remain);
            } else {
                PUT(HDRP(ptr), PACK(total, 1));
                PUT(FTRP(ptr), PACK(total, 1));
            }
            return ptr;
    }

    // heap 확장해서 in-place realloc 시도
    if (!next_alloc && next_size == 0) {  // epilogue 블록인 경우
        size_t extend_size = asize - old_size;
        void *new_area = mem_sbrk(extend_size);
        if ((long)new_area != -1) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1)); // epilogue update
            return ptr;
        }
    }

    // fallback: 새로 할당하고 copy
    void *newptr = mm_malloc(size);
    if (newptr == NULL) return NULL;

    size_t copySize = old_size - DSIZE;
    if (size < copySize) copySize = size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}


/*            추가 구현 함수         */
static int class_index(size_t size) {
    if (size <= 16) return 0;
    else if (size <= 32) return 1;
    else if (size <= 64) return 2;
    else if (size <= 128) return 3;
    else if (size <= 256) return 4;
    else if (size <= 512) return 5;
    else if (size <= 1024) return 6;
    else if (size <= 2048) return 7;
    else if (size <= 4096) return 8;
    else return 9;
}

static void insert_free_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int i = class_index(size);

    void *head = seg_free_lists[i];
    SUCC(bp) = head;
    PRED(bp) = NULL;

    if (head != NULL)
        PRED(head) = bp;

    seg_free_lists[i] = bp;
}



static void remove_free_block(void *bp) {
    int i = class_index(GET_SIZE(HDRP(bp)));
    if (GET_ALLOC(HDRP(bp))) {
        fprintf(stderr, "remove_free_block error: trying to remove allocated block %p\n", bp);
        exit(1);
    }
    if (seg_free_lists[i] == NULL) return;
     // 첫 번째 노드인 경우
    if (seg_free_lists[i] == bp) {
        seg_free_lists[i] = SUCC(bp);
        if (SUCC(bp)) PRED(SUCC(bp)) = NULL;
    } else {
        // 중간이나 끝 노드인 경우
        if (PRED(bp)) SUCC(PRED(bp)) = SUCC(bp);
        if (SUCC(bp)) PRED(SUCC(bp)) = PRED(bp);
    }
    
    // 포인터 초기화
    PRED(bp) = NULL;
    SUCC(bp) = NULL;
}

static void *coalesce(void *bp) {
    if (bp == NULL) {
    fprintf(stderr, "coalesce called with NULL\\n");
    exit(1);
    }

    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (!prev_alloc&& !GET_ALLOC(HDRP(PREV_BLKP(bp)))) remove_free_block(PREV_BLKP(bp));
    if (!next_alloc&& !GET_ALLOC(HDRP(NEXT_BLKP(bp)))) remove_free_block(NEXT_BLKP(bp));

    if (!prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
    } else if (!prev_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
    } else if (!next_alloc) {
        size += GET_SIZE(FTRP(NEXT_BLKP(bp)));
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_free_block(bp);
    return bp;
}

static void *extend_heap(size_t words) {
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    void *bp = mem_sbrk(size);
    if ((long)bp == -1) return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *find_fit(size_t asize) {
    void *best_bp = NULL;
    size_t best_size = (size_t)-1 ;
    
    /* Search from appropriate size class */
    for (int class = class_index(asize); class < CLASS_NUM; class++) {
        for (void *bp = seg_free_lists[class]; bp != NULL; bp = SUCC(bp)) {
            size_t bsize = GET_SIZE(HDRP(bp));
            
            if (bsize == asize) {
                return bp; // perfect match
            }

            // split 손실 방지: 차이가 8B 이하면 그대로 사용
            if (bsize > asize && (bsize - asize) <= DSIZE) {
                return bp;
            }

            // 일반 best-fit
            if (bsize > asize && bsize < best_size) {
                best_size = bsize;
                best_bp = bp;
            }
        }
        
        /* If we found a fit in this class, use it */
        if (best_bp != NULL) {
            return best_bp;
        }
    }
    
    return NULL; /* No fit */
}

static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    remove_free_block(bp);
    if ((csize - asize) >= (4 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        void *next = NEXT_BLKP(bp);
        PUT(HDRP(next), PACK(csize - asize, 0));
        PUT(FTRP(next), PACK(csize - asize, 0));
        insert_free_block(next);
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}