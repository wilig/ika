#pragma once

#include "../../lib/allocator.h"
#include "../defines.h"
#include "stdlib.h"
#include <stdint.h>

#define DYNAMIC_ARRAY_DEFAULT_CHUNK_SIZE 5

typedef struct {
  u32 element_size;
  u64 capacity;
  u64 count;
  // How many elements to allocate at once.
  u32 chunk_size;
} dynamic_array_t;

void *i_dynamic_array_init(u32 element_size);
b8 i_dynamic_array_deinit(void *array);
dynamic_array_t *i_dynamic_array_info(void *array);

b8 i_dynamic_array_get(void *array, uint64_t slot, void *element);
void *i_dynamic_array_get_ref(void *array, u64 slot);
void *i_dynamic_array_append(void *array, void *element);
b8 i_dynamic_array_put(void *array, uint64_t slot, void *element_ptr);
void *i_dynamic_array_push(void *array, void *element_ptr);
b8 i_dynamic_array_pop(void *array, void *element);

#define darray_init(type) i_dynamic_array_init(sizeof(type))
#define darray_deinit(array) i_dynamic_array_deinit(array)
#define darray_append(array, value)                                            \
  {                                                                            \
    __typeof__(value) temp = value;                                            \
    array = i_dynamic_array_append(array, &temp);                              \
  }
#define darray_get(array, slot) i_dynamic_array_get(array, slot)
#define darray_info(array) i_dynamic_array_info(array)
#define darray_put(array, slot, element)                                       \
  i_dynamic_array_put(array, slot, element)
#define darray_push(array, element) darray_append(array, element)
#define darray_pop(array, element) i_dynamic_array_pop(array, element)
#define darray_len(array) i_dynamic_array_info(array)->count
