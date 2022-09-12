#include "dynarray.h"
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
  array->allocator = allocator;
  array->element_size = element_size;
  array->capacity = DYNARRAY_DEFAULT_CHUNK_SIZE;
  array->count = 0;
  array->chunk_size = DYNARRAY_DEFAULT_CHUNK_SIZE;
  array->storage = allocator_alloc_or_exit(
      allocator, element_size * DYNARRAY_DEFAULT_CHUNK_SIZE);
  return array;
}

void dynarray_deinit(dynarray *array) {
  allocator_free(array->allocator, array->storage);
  allocator_free(array->allocator, array);
}

void dynarray_append(dynarray *array, void *element) {
  if (array->count + 1 < array->capacity) {
    memcpy(&array->storage + (array->element_size * array->count), element,
           array->element_size);
    array->count += 1;
  } else {
    printf("dynarray expansion not yet implemented\n");
    exit(-2);
  }
}

void *dynarray_get(dynarray *array, int slot) {
  if (slot < array->capacity) {
    return (void *)(&array->storage + (array->element_size * slot));
  }
  return NULL;
}

void dynarray_put(dynarray *array, int slot, void *element) {
  if (slot < array->capacity) {
    memcpy(&array->storage + (array->element_size * slot), element,
           array->element_size);
  }
}

void dynarray_push(dynarray *array, void *element) {
  dynarray_append(array, element);
}

void *dynarray_pop_owned_element(dynarray *array) {
  void *element =
      allocator_alloc_or_exit(array->allocator, array->element_size);
  memcpy(element, &array->storage + (array->element_size * (array->count - 1)),
         array->element_size);
  array->count -= 1;
  return element;
}
