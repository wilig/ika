#pragma once

#include "defines.h"
#include "stdbool.h"

// 128k chunks
#define DEFAULT_CHUNK_SIZE 128 * 1024

typedef enum {
  linear_allocator_t,
} allocator_type;

typedef struct allocated_memory {
  void *ptr;
  uint64_t size;
  bool valid;
} allocated_memory;

typedef struct linear_allocator_options {
  uint64_t chunk_size;
} linear_allocator_options;

typedef struct allocator_options {
  linear_allocator_options linear;
} allocator_options;

typedef struct linear_allocator {
  uint8_t *bucket_o_mem;
  uint64_t chunk_size;
  uint64_t capacity;
  uint64_t free_space;
  bool valid;
} linear_allocator;

typedef struct allocator_t {
  void *allocator_internals;
  allocator_type type;
  bool valid;
} allocator_t;

allocator_t allocator_init(allocator_type, allocator_options);
void allocator_deinit(allocator_t);

allocated_memory allocator_alloc(allocator_t, uint64_t);
void *allocator_alloc_or_exit(allocator_t *, uint64_t);
allocated_memory allocator_realloc(allocator_t, allocated_memory, uint64_t);
void allocator_free(allocator_t, void *);
