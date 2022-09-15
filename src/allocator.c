#include "allocator.h"
#include "defines.h"
#include "log.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static allocator_memory_chunk_t *
linear_allocator_new_chunk(uint64_t amount_to_alloc) {
  allocator_memory_chunk_t *new_chunk =
      calloc(sizeof(allocator_memory_chunk_t), 1);
  uint8_t *mem_ptr = calloc(amount_to_alloc, 1);
  if (new_chunk != NULL && mem_ptr != NULL) { // Success
    new_chunk->mem_ptr = mem_ptr;
    new_chunk->valid = true;
    new_chunk->capacity = amount_to_alloc;
    new_chunk->free_space = amount_to_alloc;
    new_chunk->next = NULL;
    return new_chunk;
  }
  return NULL;
}

allocated_memory linear_allocator_alloc(allocator_t allocator,
                                        uint64_t bytes_to_alloc) {
  if (allocator.valid) {
    // Get internal details
    linear_allocator_t *la =
        (linear_allocator_t *)allocator.allocator_internals;
    allocator_memory_chunk_t *chunk = la->current_chunk;

    // Handle overflow
    if (bytes_to_alloc > chunk->free_space) {
      chunk = linear_allocator_new_chunk(la->chunk_size);
      if (chunk != NULL) {
        la->current_chunk->next = chunk;
        la->current_chunk = chunk;
      } else {
        return (allocated_memory){.valid = false, .ptr = 0};
      }
    }
    void *ptr = chunk->mem_ptr + (chunk->capacity - chunk->free_space);
    chunk->free_space -= bytes_to_alloc;

    return (allocated_memory){
        .ptr = ptr, .valid = true, .size = bytes_to_alloc};
  } else {
    return (allocated_memory){.ptr = 0, .valid = false};
  }
}

allocated_memory linear_allocator_realloc(allocator_t allocator,
                                          allocated_memory mem,
                                          uint64_t bytes_to_allocate) {
  allocated_memory new_mem =
      linear_allocator_alloc(allocator, mem.size + bytes_to_allocate);
  if (new_mem.valid) {
    memcpy(new_mem.ptr, mem.ptr, mem.size);
    assert(memcmp(mem.ptr, new_mem.ptr, mem.size) == 0);
  }
  return new_mem;
}

void linear_allocator_free(allocator_t self, void *mem) {}
void linear_allocator_deinit(allocator_t self) {
  if (self.valid) {
    linear_allocator_t *la = (linear_allocator_t *)self.allocator_internals;
    allocator_memory_chunk_t *head = la->head;
    allocator_memory_chunk_t *next = la->head->next;
    while (head != NULL) {
      free(head->mem_ptr);
      free(head);
      head = next;
      next = head->next;
    }
    free(la);
  }
}

allocator_t make_linear_allocator(linear_allocator_options options) {
  const uint64_t chunk_size =
      options.chunk_size == 0 ? DEFAULT_CHUNK_SIZE : options.chunk_size;
  assert(chunk_size > 0);
  linear_allocator_t *la = calloc(sizeof(linear_allocator_t), 1);
  if (la != NULL) {
    allocator_memory_chunk_t *chunk = linear_allocator_new_chunk(chunk_size);
    if (chunk != NULL) {
      la->head = chunk;
      la->current_chunk = chunk;
      la->chunk_size = chunk_size;
      return (allocator_t){.allocator_internals = la, .valid = true};
    }
  }
  log_error("Creating linear allocator, failed.");
  return (allocator_t){.valid = false};
}

allocator_t allocator_init(allocator_type type, allocator_options options) {
  switch (type) {
  case linear_allocator:
    return make_linear_allocator(options.linear);
  }
}

void allocator_deinit(allocator_t allocator) {
  switch (allocator.type) {
  case linear_allocator:
    linear_allocator_deinit(allocator);
    break;
  }
}

allocated_memory allocator_alloc(allocator_t allocator,
                                 uint64_t bytes_to_alloc) {
  switch (allocator.type) {
  case linear_allocator:
    return linear_allocator_alloc(allocator, bytes_to_alloc);
    break;
  }
}

allocated_memory allocator_realloc(allocator_t allocator, allocated_memory mem,
                                   uint64_t bytes_to_alloc) {
  switch (allocator.type) {
  case linear_allocator:
    return linear_allocator_realloc(allocator, mem, bytes_to_alloc);
    break;
  }
}

void *allocator_alloc_or_exit(allocator_t allocator, uint64_t bytes_to_alloc) {
  allocated_memory mem = allocator_alloc(allocator, bytes_to_alloc);
  if (!mem.valid) {
    log_error("Failed to allocate memory, so exiting according to contract.");
    exit(-1);
  }
  return mem.ptr;
}

void allocator_free(allocator_t allocator, void *mem) {
  switch (allocator.type) {
  case linear_allocator:
    linear_allocator_free(allocator, mem);
    break;
  }
}
