#include "darray.h"
#include "allocator.h"
#include "log.h"
#include <string.h>

// Very simple dynamic array implementation
//
// This is heavily inspired (stolen?) from the Kohi game engine.  It's an
// incredible learning resource for getting into C programming.
//

// This trick here is to allocate a block of memory, stick our stats structure
// at the very beginning and return a pointer to the memory after the stats
// structure.  This allows it to be treated as a normal array for iteration
// and indexing, but provides additional functionality implemented below.
//

void *i_dynamic_array_init(u32 element_size) {
  u64 total_size = sizeof(dynamic_array_t) +
                   (element_size * DYNAMIC_ARRAY_DEFAULT_CHUNK_SIZE);
  dynamic_array_t *stats = imust_alloc(total_size);
  stats->element_size = element_size;
  stats->chunk_size = DYNAMIC_ARRAY_DEFAULT_CHUNK_SIZE;
  stats->capacity = DYNAMIC_ARRAY_DEFAULT_CHUNK_SIZE;
  stats->count = 0;

  // Return a pointer to the memory just after our stats
  return (void *)(stats + 1);
}

dynamic_array_t *i_dynamic_array_info(void *array) {
  return ((dynamic_array_t *)array - 1);
}

// Frees all the memory associated with the array.
b8 i_dynamic_array_deinit(void *array) {
  dynamic_array_t *stats = i_dynamic_array_info(array);
  ifree(stats);
  return TRUE;
}

// Adds a copy of the element parameter to the end of the array
void *i_dynamic_array_append(void *array, void *element) {
  dynamic_array_t *stats = i_dynamic_array_info(array);
  if (stats->count < stats->capacity) {
    int8_t *storage_location =
        ((int8_t *)array + (stats->element_size * stats->count));
    memcpy(storage_location, element, stats->element_size);
    stats->count += 1;
    return array;
  } else {
    // When we run out of capacity, we allocate addition space based on current
    // usage plus our default chunk size.  We then copy the from the old
    // storage to the new storage.
    u64 bytes_to_allocate =
        sizeof(dynamic_array_t) +
        ((stats->capacity + stats->chunk_size) * stats->element_size);
    u64 bytes_to_copy = stats->capacity * stats->element_size;
    void *previous_storage = array;
    int8_t *new_loc = imust_alloc(bytes_to_allocate);
    // Update the stats, before we copy them over.
    stats->capacity += stats->chunk_size;
    // Copy stats header
    memcpy(new_loc, stats, sizeof(dynamic_array_t));
    // Copy existing array entries
    void *new_array = (void *)(new_loc + sizeof(dynamic_array_t));
    memcpy(new_array, previous_storage, bytes_to_copy);
    return i_dynamic_array_append(new_array, element);
  }
}

// Returns a pointer to the element in the array.  You must cast the pointer
// to the appropriate type.
void *i_dynamic_array_get_ref(void *array, u64 slot) {
  dynamic_array_t *stats = i_dynamic_array_info(array);
  if (slot < stats->count) {
    return (void *)((int8_t *)array + (stats->element_size * slot));
  }
  return NULL;
}

// Copies the element parameter and overwrites the element in the
// specified position.
b8 i_dynamic_array_put(void *array, uint64_t slot, void *element) {
  dynamic_array_t *stats = i_dynamic_array_info(array);
  if (slot < stats->count) {
    memcpy((int8_t *)array + (stats->element_size * slot), element,
           stats->element_size);
    return TRUE;
  }
  return FALSE;
}

// Synonym for append
void *i_dynamic_array_push(void *array, void *element) {
  return i_dynamic_array_append(array, element);
}

// Removes the last element from the array, and returns it.  Caller
// is responsible for freeing the memory.
b8 i_dynamic_array_pop(void *array, void *out_element) {
  dynamic_array_t *stats = i_dynamic_array_info(array);
  memcpy(out_element,
         (uint8_t *)array + (stats->element_size * (stats->count - 1)),
         stats->element_size);
  stats->count -= 1;
  return TRUE;
}
