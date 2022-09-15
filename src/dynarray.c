#include "dynarray.h"
#include "allocator.h"
#include "log.h"
#include <string.h>

// Very simple dynamic array implementation
//
// We allocate storage based on the size of the type that will be stored in
// the array.  It's all pointer magic from there.  There are probably better
// ways to get this done, but this is what I'm settling on until I learn
// better.
dynarray *dynarray_init(allocator_t allocator, size_t element_size) {
  dynarray *array = allocator_alloc_or_exit(allocator, sizeof(dynarray));
  void *storage = allocator_alloc_or_exit(
      allocator, element_size * DYNARRAY_DEFAULT_CHUNK_SIZE);
  printf("dynarray_init: base array points to %p\n", array);
  array->allocator = allocator;
  array->element_size = element_size;
  array->capacity = DYNARRAY_DEFAULT_CHUNK_SIZE;
  array->count = 0;
  array->chunk_size = DYNARRAY_DEFAULT_CHUNK_SIZE;
  array->storage = storage;

  printf("dynarray_init: count: %li\n", array->count);
  printf("dynarray_init: memory ptr: %p\n", array->storage);
  printf("dynarray_init: count: %li\n", array->count);
  return array;
}

void dynarray_deinit(dynarray *array) {
  allocator_free(array->allocator, array->storage);
  allocator_free(array->allocator, array);
}

void dynarray_append(dynarray *array, void *element) {
  if (array->count + 1 < array->capacity) {
    void *storage = array->storage;
    //   memcpy(storage + (array->element_size * array->count), element,
    //          array->element_size);
    array->count += 1;
    printf("dynarray_append: array->capacity: %li\n", array->capacity);
    printf("dynarray_append: after memcpy array->count: %li\n", array->count);
  } else {
    printf(
        "BEFORE REALLOC:\n------------------------------------------------\n");
    printf("array points to %p\n", array);
    printf("array->storage points to %p\n", array->storage);
    printf("array->capacity before realloc: %li\n", array->capacity);
    printf("array->count before realloc: %li\n", array->count);
    printf("array->element_size before realloc: %li\n", array->element_size);
    printf("array->chunk_size before realloc: %li\n", array->chunk_size);

    array->storage = allocator_alloc_or_exit(
        array->allocator,
        array->capacity + array->element_size * DYNARRAY_DEFAULT_CHUNK_SIZE);
    array->capacity += DYNARRAY_DEFAULT_CHUNK_SIZE;
    printf(
        "AFTER REALLOC:\n------------------------------------------------\n");
    printf("array points to %p\n", array);
    printf("array->storage points to %p\n", array->storage);
    printf("array->capacity after realloc: %li\n", array->capacity);
    printf("array->count after realloc: %li\n", array->count);
    printf("array->element_size after realloc: %li\n", array->element_size);
    printf("array->chunk_size after realloc: %li\n", array->chunk_size);

    if (array->storage == NULL) {
      printf("dynarray expansion failed\n");
      exit(-2);
    }
    printf("Calling append again\n");
    return dynarray_append(array, element);
  }
}

void *dynarray_get(dynarray *array, int slot) {
  if (slot < array->capacity) {
    return (void *)(array->storage + (array->element_size * slot));
  }
  return NULL;
}

void dynarray_put(dynarray *array, int slot, void *element) {
  if (slot < array->capacity) {
    //    memcpy(array->storage + (array->element_size * slot), element,
    //           array->element_size);
  }
}

void dynarray_push(dynarray *array, void *element) {
  dynarray_append(array, element);
}

void *dynarray_pop_owned_element(dynarray *array) {
  void *element =
      allocator_alloc_or_exit(array->allocator, array->element_size);
  memcpy(element, array->storage + (array->element_size * (array->count - 1)),
         array->element_size);
  array->count -= 1;
  return element;
}
