#pragma once

#include "defines.h"

// 128k chunks
#define DEFAULT_CHUNK_SIZE 128 * 1024

typedef struct allocator_memory_chunk_t {
  i8 *mem_ptr;
  u64 capacity;
  u64 free_space;
  b8 valid;
  struct allocator_memory_chunk_t *next;
} allocator_memory_chunk_t;

typedef struct linear_allocator_t {
  allocator_memory_chunk_t *head;
  allocator_memory_chunk_t *current_chunk;
  uint64_t chunk_size;
} linear_allocator_t;

b8 initialize_allocator();
void shutdown_allocator();

void *imust_alloc(u64 bytes);
void *ialloc(u64 bytes);
void ifree(void *mem_ptr);
