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
  array->allocator = allocator;
  array->element_size = element_size;
  array->capacity = DYNARRAY_DEFAULT_CHUNK_SIZE;
  array->count = 0;
  array->chunk_size = DYNARRAY_DEFAULT_CHUNK_SIZE;
  array->storage = storage;

  return array;
}

// Frees all the memory associated with the array.
void dynarray_deinit(dynarray *array) {
  allocator_free(array->allocator, array->storage);
  allocator_free(array->allocator, array);
}

// Adds a copy of the element parameter to the end of the array
void dynarray_append(dynarray *array, void *element) {
  if (array->count < array->capacity) {
    int8_t *storage = array->storage;
    int8_t *storage_location = (storage + (array->element_size * array->count));
    memcpy(storage_location, element, array->element_size);
    array->count += 1;
  } else {
    // When we run out of capacity, we allocate addition space based on current
    // usage plus our default chunk size.  We then copy the from the old
    // storage to the new storage.
    uint64_t bytes_to_allocate =
        (array->capacity + DYNARRAY_DEFAULT_CHUNK_SIZE) * array->element_size;
    void *previous_storage = array->storage;
    array->storage =
        allocator_alloc_or_exit(array->allocator, bytes_to_allocate);
    array->capacity += DYNARRAY_DEFAULT_CHUNK_SIZE;
    memcpy(array->storage, previous_storage, bytes_to_allocate);
    if (array->storage == NULL) {
      printf("dynarray expansion failed\n");
      exit(-2);
    }
    return dynarray_append(array, element);
  }
}

// Returns a pointer to the element in the array.  You must cast the pointer
// to the appropriate type.
void *dynarray_get(dynarray *array, uint64_t slot) {
  if (slot < array->capacity) {
    return (void *)(array->storage + (array->element_size * slot));
  }
  return NULL;
}

// Copies the element parameter and overwrites the element in the
// specified position.
void dynarray_put(dynarray *array, uint64_t slot, void *element) {
  if (slot < array->capacity) {
    memcpy(array->storage + (array->element_size * slot), element,
           array->element_size);
  }
}

// Synonym for append
void dynarray_push(dynarray *array, void *element) {
  dynarray_append(array, element);
}

// Removes the last element from the array, and returns it.  Caller
// is responsible for freeing the memory.
void *dynarray_pop_owned_element(dynarray *array) {
  void *element =
      allocator_alloc_or_exit(array->allocator, array->element_size);
  memcpy(element, array->storage + (array->element_size * (array->count - 1)),
         array->element_size);
  array->count -= 1;
  return element;
}
