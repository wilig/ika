#pragma once

#include "allocator.h"
#include "stdlib.h"

#define DYNARRAY_DEFAULT_CHUNK_SIZE 25

typedef struct {
  allocator_t allocator;
  size_t element_size;
  size_t capacity;
  size_t count;
  // How many elements to allocate at once.
  size_t chunk_size;
  // Pointer to allocated memory pool
  void *storage;
} dynarray;

dynarray *dynarray_init(allocator_t allocator, size_t element_size);
void dynarray_deinit(dynarray *array);
void dynarray_append(dynarray *array, void *element);

void *dynarray_get(dynarray *array, int slot);
void dynarray_put(dynarray *array, int slot, void *element);
void dynarray_push(dynarray *array, void *element);
void *dynarray_pop_owned_element(dynarray *array);
