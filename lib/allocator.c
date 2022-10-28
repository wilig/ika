#include "allocator.h"
#include "defines.h"
#include "log.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static linear_allocator_t *root_allocator = NULL;

static allocator_memory_chunk_t *
linear_allocator_new_chunk(uint64_t amount_to_alloc) {
  allocator_memory_chunk_t *new_chunk =
      calloc(sizeof(allocator_memory_chunk_t), 1);
  int8_t *mem_ptr = calloc(amount_to_alloc, 1);
  if (new_chunk != NULL && mem_ptr != NULL) { // Success
    new_chunk->mem_ptr = mem_ptr;
    new_chunk->valid = TRUE;
    new_chunk->capacity = amount_to_alloc;
    new_chunk->free_space = amount_to_alloc;
    new_chunk->next = NULL;
    return new_chunk;
  }
  return NULL;
}

void *imust_alloc(u64 bytes) {
  if (root_allocator) {
    void *mem_ptr = ialloc(bytes);
    if (mem_ptr) {
      return mem_ptr;
    } else {
      FATAL("Could not allocate %li bytes of memory\n", bytes);
      return NULL;
    }
  } else {
    WARN("Allocation requested before allocator was initialized.  Using raw "
         "calloc.");
    return calloc(bytes, 1);
  }
}

void *ialloc(u64 bytes) {
  if (root_allocator) {
    allocator_memory_chunk_t *chunk = root_allocator->current_chunk;

    // Handle overflow
    if (bytes > chunk->free_space) {
      chunk = bytes > root_allocator->chunk_size
                  ? linear_allocator_new_chunk(bytes)
                  : linear_allocator_new_chunk(root_allocator->chunk_size);
      if (chunk != NULL) {
        root_allocator->current_chunk->next = chunk;
        root_allocator->current_chunk = chunk;
      } else {
        return NULL;
      }
    }
    void *mem_ptr = chunk->mem_ptr + (chunk->capacity - chunk->free_space);
    // Clear the memory
    memset(mem_ptr, 0x0, bytes);
    chunk->free_space -= bytes;
    return mem_ptr;
  } else {
    WARN("Allocation requested before allocator was initialized.  Using raw "
         "calloc.");
    return calloc(bytes, 1);
  }
}

// A no-op for now
void ifree(void *mem) {}

void shutdown_allocator() {
  if (root_allocator) {
    allocator_memory_chunk_t *head = root_allocator->head;
    allocator_memory_chunk_t *next = root_allocator->head->next;
    while (head != NULL) {
      free(head->mem_ptr);
      free(head);
      head = next;
      next = head->next;
    }
    free(root_allocator);
  }
}

b8 initialize_allocator() {
  root_allocator = calloc(sizeof(linear_allocator_t), 1);
  if (root_allocator != NULL) {
    allocator_memory_chunk_t *chunk =
        linear_allocator_new_chunk(DEFAULT_CHUNK_SIZE);
    if (chunk != NULL) {
      root_allocator->head = chunk;
      root_allocator->current_chunk = chunk;
      root_allocator->chunk_size = DEFAULT_CHUNK_SIZE;
      return TRUE;
    }
  }
  return FALSE;
}
