#ifndef MIN_PRIORITY_QUEUE_H
#define MIN_PRIORITY_QUEUE_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct PointerPriorityQueue* PtrPQ;

/**
 * Create a new minimum priority queue.
 * 
 * @param comparator Function to compare two elements (return negative if a < b, 0 if equal, positive if a > b)
 * @return A new priority queue or NULL if allocation fails
 */
PtrPQ ptr_pq_create(int (*comparator)(void *, void *));

/**
 * Free all resources associated with the priority queue.
 * Note: This does not free the pointer data itself, only the queue structure.
 * 
 * @param pq The priority queue to free
 */
void ptr_pq_free(PtrPQ pq);

/**
 * Insert an element into the priority queue.
 * 
 * @param pq The priority queue
 * @param data The void pointer to store
 * @return true if successful, false if allocation failed
 */
bool ptr_pq_insert(PtrPQ pq, void* data);

/**
 * Remove and return the minimum element from the priority queue.
 * 
 * @param pq The priority queue
 * @return The minimum element or NULL if the queue is empty
 */
void* ptr_pq_extract_min(PtrPQ pq);

/**
 * Peek at the minimum element without removing it.
 * 
 * @param pq The priority queue
 * @return The minimum element or NULL if the queue is empty
 */
void* ptr_pq_peek(PtrPQ pq);

/**
 * Check if the priority queue is empty.
 * 
 * @param pq The priority queue
 * @return true if empty, false otherwise
 */
bool ptr_pq_is_empty(PtrPQ pq);

/**
 * Get the number of elements in the priority queue.
 * 
 * @param pq The priority queue
 * @return The number of elements
 */
size_t ptr_pq_size(PtrPQ pq);

#endif /* MIN_PRIORITY_QUEUE_H */