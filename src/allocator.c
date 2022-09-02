#include "allocator.h"
#include "defines.h"
#include "log.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

allocated_memory linear_allocator_expand(linear_allocator *la,
                                         uint64_t amount_to_alloc) {
  int num_of_chunks_needed = (int)(amount_to_alloc / la->chunk_size) + 1;
  la->capacity = la->capacity + (num_of_chunks_needed * la->chunk_size);
  uint8_t *new_bucket = realloc(la->bucket_o_mem, la->capacity);
  if (new_bucket == NULL) {
    return (allocated_memory){.valid = false, .ptr = 0};
  } else {
    return (allocated_memory){.valid = true, .ptr = new_bucket};
  }
}

allocated_memory linear_allocator_alloc(allocator_t allocator,
                                        uint64_t bytes_to_alloc) {
  if (allocator.valid) {
    // Get internal details
    linear_allocator *la = (linear_allocator *)allocator.allocator_internals;
    // Handle overflow
    if (bytes_to_alloc > la->capacity - la->free_space) {
      log_warn("Expanding allocation pool\n");
      allocated_memory new_bucket = linear_allocator_expand(la, bytes_to_alloc);
      if (new_bucket.valid == true) {
        la->bucket_o_mem = new_bucket.ptr;
      } else {
        return (allocated_memory){.valid = false, .ptr = 0};
      }
    }
    void *ptr = la->bucket_o_mem + la->free_space;
    la->free_space += bytes_to_alloc;

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
  }
  return new_mem;
}

void linear_allocator_free(allocator_t self, void *mem) {}
void linear_allocator_deinit(allocator_t self) {
  if (self.valid) {
    linear_allocator *la = (linear_allocator *)self.allocator_internals;
    free(la->bucket_o_mem);
  }
}

allocator_t make_linear_allocator(linear_allocator_options options) {
  const uint64_t chunk_size =
      options.chunk_size == 0 ? DEFAULT_CHUNK_SIZE : options.chunk_size;
  assert(chunk_size > 0);
  linear_allocator *la = malloc(sizeof(linear_allocator));
  if (la != NULL) {
    la->bucket_o_mem = malloc(chunk_size);
    la->capacity = chunk_size;
    la->chunk_size = chunk_size;
    la->free_space = 0;
    la->valid = true;
    if (la->bucket_o_mem != NULL) {
      return (allocator_t){.allocator_internals = la, .valid = true};
    }
  }
  log_error("Creating linear allocator, failed.");
  return (allocator_t){.valid = false};
}

allocator_t allocator_init(allocator_type type, allocator_options options) {
  switch (type) {
  case linear_allocator_t:
    return make_linear_allocator(options.linear);
  }
}

void allocator_deinit(allocator_t allocator) {
  switch (allocator.type) {
  case linear_allocator_t:
    linear_allocator_deinit(allocator);
    break;
  }
}

allocated_memory allocator_alloc(allocator_t allocator,
                                 uint64_t bytes_to_alloc) {
  switch (allocator.type) {
  case linear_allocator_t:
    return linear_allocator_alloc(allocator, bytes_to_alloc);
    break;
  }
}

allocated_memory allocator_realloc(allocator_t allocator, allocated_memory mem,
                                   uint64_t bytes_to_alloc) {
  switch (allocator.type) {
  case linear_allocator_t:
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
  case linear_allocator_t:
    linear_allocator_free(allocator, mem);
    break;
  }
}
