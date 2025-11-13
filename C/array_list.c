#include<stdlib.h>
#include<stdio.h>
#include "array_list.h"
#include "utils.h"
#include<stdbool.h>


// We would create an array with init_length and when it goes beyond init_length, we'll try to use realloc and double the size
// Need List of pointers to keep track of multiple memory locations when realloc fails

// Internal linked-list structure for handling blocks of arraylist in memory since it might not be possible 
// to allocate contiguous block in memory as the size grows
struct ArrayBlock {
    void** dataPointers;
    int size;
    int storedLength;
    struct ArrayBlock* next;
    struct ArrayBlock* prev;
};

// Externally exposed structure for arrayList
struct ArrayList {
    int length;
    int elementSize;
    struct ArrayBlock* arrayStartBlock;
    struct ArrayBlock* arrayEndBlock;
};

enum ArrayListOperationResponseCode {
    ARRAY_LIST_SUCCESS,
    ARRAY_LIST_MEMORY_ALLOCATION_FAILURE,
    ARRAY_LIST_BAD_OPERATION,
    ARRAY_LIST_HEAP_COPY_FAILURE
};

// Externally exposed structure for arraylist operations
struct ArrayListOperationResponse {
    struct ArrayList* arrayList;
    enum ArrayListOperationResponseCode responseCode;
};

struct ArrayListOperationResponse createResponse(struct ArrayList* arrayList, enum ArrayListOperationResponseCode
     responseCode) {
    struct ArrayListOperationResponse response;
    response.arrayList = arrayList;
    response.responseCode = responseCode;
    return response;
}

struct ArrayListOperationResponse createArrayList(struct ArrayBlock* initArrayBlock, int length, int elementSize) {
    struct ArrayList* arrayList = (struct ArrayList*) malloc(sizeof(struct ArrayList));
    if (arrayList == NULL) {
        return createResponse(NULL, ARRAY_LIST_MEMORY_ALLOCATION_FAILURE);
    }
    arrayList->arrayStartBlock = initArrayBlock;
    arrayList->arrayEndBlock = initArrayBlock;
    arrayList->length = length;
    arrayList->elementSize = elementSize;
    return createResponse(arrayList, ARRAY_LIST_SUCCESS);
}

int createDataPointerBlock(int blockSize, void*** blockDataPointer) {
    *blockDataPointer = NULL;
    while(blockSize > 0) {
        *blockDataPointer = (void**) malloc(sizeof(void *) * blockSize);
        if(*blockDataPointer != NULL) {
            return blockSize;
        } else {
            blockSize = blockSize / 2;
        }
    }
    return blockSize;
}

struct ArrayBlock* createArrayBlock(int blockSize) {
    struct ArrayBlock *arrayBlock = (struct ArrayBlock*) malloc (sizeof(struct ArrayBlock));
    // Memory Allocation issue
    if (arrayBlock == NULL) {
        return NULL;
    }
    void **blockDataPointer;
    int createdBlockSize = createDataPointerBlock(blockSize, &blockDataPointer);
    if (createdBlockSize == 0) { // Memory Allocation issue
        free(arrayBlock);
        return NULL;
    }
    arrayBlock->dataPointers = blockDataPointer;
    arrayBlock->size = createdBlockSize;
    arrayBlock->storedLength = 0;
    arrayBlock->next = NULL;
    arrayBlock->prev = NULL;
    return arrayBlock;
}

bool increaseArrayListSize(struct ArrayList* arrayList) {
    int newSize = arrayList->arrayEndBlock->size * 2;
    void** resizedPointer = (void **) realloc(arrayList->arrayEndBlock->dataPointers, sizeof(void *) * newSize);
    
    if (resizedPointer != NULL) {
        arrayList->arrayEndBlock->dataPointers = resizedPointer;
        arrayList->arrayEndBlock->size = newSize;
        return true;
    }

    struct ArrayBlock* newBlock = createArrayBlock(arrayList->arrayEndBlock->size);
    if (newBlock != NULL) {
        arrayList->arrayEndBlock->next = newBlock;
        newBlock->prev = arrayList->arrayEndBlock;
        arrayList->arrayEndBlock = newBlock;
        return true;
    }
    return false;
}

struct ArrayListOperationResponse arraylist_create(int init_length, int elementSize) {
    struct ArrayBlock *initArrayBlock = createArrayBlock(init_length);
    if (initArrayBlock == NULL) {
        return createResponse(NULL, ARRAY_LIST_MEMORY_ALLOCATION_FAILURE);
    }
    return createArrayList(initArrayBlock, 0, elementSize);
}

struct ArrayListOperationResponse arraylist_insert(struct ArrayList* arraylist, void* data, int index) {
    void *copiedDataPointer = copy_to_heap(data, arraylist->elementSize);

    if (copiedDataPointer == NULL) {
        return createResponse(arraylist, ARRAY_LIST_HEAP_COPY_FAILURE);
    }
    
    if (index > arraylist->length) {
        return createResponse(arraylist, ARRAY_LIST_BAD_OPERATION);
    }

    if (arraylist->arrayEndBlock->size == arraylist->arrayEndBlock->storedLength) {
        if(!increaseArrayListSize(arraylist)) {
            return createResponse(arraylist, ARRAY_LIST_MEMORY_ALLOCATION_FAILURE);
        }
    }

    struct ArrayBlock* traversalBlock = arraylist->arrayStartBlock;
    while (traversalBlock != NULL && traversalBlock->size < index + 1) {
        index -= traversalBlock->size;
        traversalBlock = traversalBlock->next;
    }

    // In the index, insert the value and move all other values to next indexes
    void* prevDataPointer = copiedDataPointer;
    do {
        int blockLength = traversalBlock->storedLength;
        traversalBlock->next == NULL && blockLength++;
        for(int i=index; i<blockLength; i++) {
            void* temp = traversalBlock->dataPointers[i];
            traversalBlock->dataPointers[i] = prevDataPointer;
            prevDataPointer = temp;
        }
        traversalBlock = traversalBlock->next;
        index = 0;
    } while(traversalBlock != NULL);

    arraylist->length++;
    arraylist->arrayEndBlock->storedLength++;
    return createResponse(arraylist, ARRAY_LIST_SUCCESS);
}

/*
    The size of the arrayList would be the index to append a new value
    The arrayList will have multiple blocks (potentially) which will be in a linked list data structure
*/
struct ArrayListOperationResponse arraylist_append(struct ArrayList* arraylist, void* data) {
    void *copiedDataPointer = copy_to_heap(data, arraylist->elementSize);

    if (copiedDataPointer == NULL) {
        return createResponse(arraylist, ARRAY_LIST_HEAP_COPY_FAILURE);
    }

    if (arraylist->arrayEndBlock->size == arraylist->arrayEndBlock->storedLength) {
        if(!increaseArrayListSize(arraylist)) {
            return createResponse(arraylist, ARRAY_LIST_MEMORY_ALLOCATION_FAILURE);
        }
    }
    arraylist->arrayEndBlock->dataPointers[arraylist->arrayEndBlock->storedLength] = copiedDataPointer;
    arraylist->arrayEndBlock->storedLength++;
    arraylist->length++;
    return createResponse(arraylist, ARRAY_LIST_SUCCESS);
}

struct ArrayListOperationResponse arraylist_delete(struct ArrayList* arraylist, int index) {
    if(index >= arraylist->length) {
        return createResponse(arraylist, ARRAY_LIST_BAD_OPERATION);
    }
    int reversedIndex = arraylist->length - index;
    struct ArrayBlock* traversalBlock = arraylist->arrayEndBlock;
    void* nextDataPointer = NULL;
    
    while (traversalBlock != NULL && reversedIndex > 0) {
        for(int i = traversalBlock->storedLength - 1; i >= 0 && reversedIndex > 0; i--, reversedIndex--) {
            void* temp = traversalBlock->dataPointers[i];
            traversalBlock->dataPointers[i] = nextDataPointer;
            nextDataPointer = temp;
        }
        traversalBlock = traversalBlock->prev;
    }
    free(nextDataPointer);
    arraylist->length--;
    arraylist->arrayEndBlock->storedLength--;
}

void* arraylist_get(struct ArrayList* arraylist, int index) {
    struct ArrayBlock * arrayBlock = arraylist->arrayStartBlock;

    while (arrayBlock != NULL && arrayBlock->size <= index) {
        index = index - arrayBlock->size;
        arrayBlock = arrayBlock->next;
    }

    return arrayBlock->dataPointers[index];
}

void arraylist_free(struct ArrayList* arraylist) {
    if (arraylist == NULL) return;
    struct ArrayBlock* traversalBlock = arraylist->arrayStartBlock;
    while (traversalBlock != NULL) {
        struct ArrayBlock* next = traversalBlock->next;
        for(int i=0; i<traversalBlock->storedLength; i++) {
            free(traversalBlock->dataPointers[i]);
        }
        free(traversalBlock->dataPointers);
        free(traversalBlock);
        traversalBlock = next;
    }
    free(arraylist);
}

void test() {
    struct ArrayListOperationResponse response = arraylist_create(1, sizeof(char));
    struct ArrayList *arraylist = response.arrayList;

    for(char i='A'; i<='Z'; i+=2) {
        struct ArrayListOperationResponse response = arraylist_append(arraylist, &i);
        if (response.responseCode != ARRAY_LIST_SUCCESS) {
            printf("\nAppend failed with code: %d", response.responseCode);
        }
    }
    printf("\nAppending Again \n");
    for(char i='B'; i<='Z'; i+=2) {
        arraylist_insert(arraylist, &i, (int)i-'A');
    }

    printf("\nTotalSize: %d, Deleting %c\n", arraylist->length, *(char*)arraylist_get(arraylist, 24));
    arraylist_delete(arraylist, 1);
    // arraylist_delete(arraylist, arraylist->length);

    for(int i=0; i<arraylist->length; i++) {
        printf("%c ", *(char *)arraylist_get(arraylist, i));
    }
}
