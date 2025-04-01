#include "ptr_pq.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

struct PointerPriorityQueue
{
    void **heap;                                   // Array of void pointers
    size_t size;                                   // Current number of elements
    size_t capacity;                               // Current capacity
    int (*comparator)(void *, void *); // Function to compare elements
};

static void swap(void **a, void **b);
static void heapify_up(PtrPQ pq, size_t index);
static void heapify_down(PtrPQ pq, size_t index);
static bool resize_if_needed(PtrPQ pq);

PtrPQ ptr_pq_create(int (*comparator)(void *, void *))
{
    if (!comparator)
    {
        return NULL;
    }

    PtrPQ pq = (PtrPQ)malloc(sizeof *pq);
    if (!pq)
    {
        return NULL;
    }

    pq->heap = (void **)malloc(INITIAL_CAPACITY * sizeof(void *));
    if (!pq->heap)
    {
        free(pq);
        return NULL;
    }

    pq->size = 0;
    pq->capacity = INITIAL_CAPACITY;
    pq->comparator = comparator;

    return pq;
}

void ptr_pq_free(PtrPQ pq)
{
    if (pq)
    {
        free(pq->heap);
        free(pq);
    }
}

bool ptr_pq_insert(PtrPQ pq, void *data)
{
    if (!pq)
    {
        return false;
    }

    if (!resize_if_needed(pq))
    {
        return false;
    }

    pq->heap[pq->size] = data;
    heapify_up(pq, pq->size);
    pq->size++;

    return true;
}

void *ptr_pq_extract_min(PtrPQ pq)
{
    if (!pq || pq->size == 0)
    {
        return NULL;
    }

    void *min = pq->heap[0];
    pq->size--;

    if (pq->size > 0)
    {
        pq->heap[0] = pq->heap[pq->size];
        heapify_down(pq, 0);
    }

    return min;
}

void *ptr_pq_peek(PtrPQ pq)
{
    if (!pq || pq->size == 0)
    {
        return NULL;
    }

    return pq->heap[0];
}

bool ptr_pq_is_empty(PtrPQ pq)
{
    return pq ? pq->size == 0 : true;
}

size_t ptr_pq_size(PtrPQ pq)
{
    return pq ? pq->size : 0;
}

///// Private Functions /////

static void swap(void **a, void **b)
{
    void *temp = *a;
    *a = *b;
    *b = temp;
}

static void heapify_up(PtrPQ pq, size_t index)
{
    if (index == 0)
        return;

    size_t parent = (index - 1) / 2;

    if (pq->comparator(pq->heap[index], pq->heap[parent]) < 0)
    {
        swap(&pq->heap[index], &pq->heap[parent]);
        heapify_up(pq, parent);
    }
}

static void heapify_down(PtrPQ pq, size_t index)
{
    size_t left_child = 2 * index + 1;
    size_t right_child = 2 * index + 2;
    size_t smallest = index;

    if (left_child < pq->size &&
        pq->comparator(pq->heap[left_child], pq->heap[smallest]) < 0)
    {
        smallest = left_child;
    }

    if (right_child < pq->size &&
        pq->comparator(pq->heap[right_child], pq->heap[smallest]) < 0)
    {
        smallest = right_child;
    }

    if (smallest != index)
    {
        swap(&pq->heap[index], &pq->heap[smallest]);
        heapify_down(pq, smallest);
    }
}

static bool resize_if_needed(PtrPQ pq)
{
    if (pq->size >= pq->capacity)
    {
        size_t new_capacity = pq->capacity * 2;
        void **new_heap = (void **)realloc(pq->heap, new_capacity * sizeof(void *));

        if (!new_heap)
        {
            return false;
        }

        pq->heap = new_heap;
        pq->capacity = new_capacity;
    }

    return true;
}
