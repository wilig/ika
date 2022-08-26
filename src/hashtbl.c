#include "hashtbl.h"
#include "allocator.h"
#include "hashing.h"
#include "log.h"
#include <string.h>

uint32_t str_hash(str s, uint64_t table_capacity) {
  uint32_t seed = 0xC0FFEE;
  uint32_t hash;
  MurmurHash3_x86_32(s.ptr, s.length, seed, &hash);
  return hash % table_capacity;
}

bool hashtbl_str_insert(hashtbl_str_t ht, str_entry_t entry) {
  allocated_memory mem = allocator_alloc(ht.allocator, sizeof(str_entry_t));
  if (!mem.valid) {
    log_error("failed to allocate memory to store hash table entry");
    return false;
  }

  str_entry_t *new_entry = mem.ptr;
  memcpy(new_entry, &entry, sizeof(str_entry_t));

  uint32_t idx = str_hash(entry.key, ht.capacity);
  log_debug("{s} hashed to index {d}.", entry.key, idx);

  if (ht.entries[idx] == NULL) {
    ht.entries[idx] = new_entry;
    return true;
  } else { // Handle collision
    log_info("Hash collision, adding item to list.");
    str_entry_t *existing_entry = ht.entries[idx];
    while (existing_entry->next != NULL &&
           !str_eq(existing_entry->key, entry.key)) {
      existing_entry = existing_entry->next;
    }
    if (existing_entry->next != NULL) {
      log_info(
          "Key already in hashtable, delete first if you want to replace it.");
      return false;
    } else {
      existing_entry->next = new_entry;
      return true;
    }
  }
}

str_entry_t hashtbl_str_lookup(hashtbl_str_t ht, str key) {
  uint32_t idx = str_hash(key, ht.capacity);

  if (ht.entries[idx] != NULL) {
    str_entry_t *entry = ht.entries[idx];
    while (!str_eq(entry->key, key) && entry->next != NULL) {
      entry = entry->next;
    }
    if (str_eq(entry->key, key)) {
      return *entry;
    } else {
      return (str_entry_t){.valid = false};
    }
  } else {
    return (str_entry_t){.valid = false};
  }
}

bool hashtbl_str_remove(hashtbl_str_t ht, str key) {
  uint32_t idx = str_hash(key, ht.capacity);
  log_debug("{s} hashed to index {d}.", key, idx);

  if (ht.entries[idx] != NULL) {
    str_entry_t *entry = ht.entries[idx];
    // Simple case, it's the first match
    if (str_eq(entry->key, key)) {
      if (entry->next != NULL) {
        ht.entries[idx] = entry->next;
      } else {
        ht.entries[idx] = NULL;
      }
      allocator_free(ht.allocator, entry);
      return true;
    }
    // Let's go looking
    str_entry_t *next_entry = entry;
    while (!str_eq(next_entry->key, key) && next_entry->next != NULL) {
      entry = next_entry;
      next_entry = next_entry->next;
    }
    if (str_eq(next_entry->key, key)) {
      entry->next = next_entry->next;
      allocator_free(ht.allocator, next_entry);
      return true;
    }
  }
  return false;
}

// Here lies a hashtable, that needs implementation
hashtbl_str_t hashtbl_str_init(allocator_t allocator) {
  allocated_memory mem =
      allocator_alloc(allocator, sizeof(str_entry_t) * DEFAULT_CAPACITY);
  if (!mem.valid) {
    return (hashtbl_str_t){.valid = false};
  }
  return (hashtbl_str_t){
      .allocator = allocator,
      .entries = mem.ptr,
      .valid = true,
      .capacity = DEFAULT_CAPACITY,
  };
}
