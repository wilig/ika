#include "hashtbl.h"
#include "allocator.h"
#include "hashing.h"
#include "log.h"
#include <assert.h>
#include <string.h>

uint32_t str_hash(str s, uint64_t table_capacity) {
  uint32_t seed = 0xC0FFEE;
  uint32_t hash;
  MurmurHash3_x86_32(s.ptr, s.length, seed, &hash);
  return hash % table_capacity;
}

bool hashtbl_str_insert(hashtbl_str_t *ht, str_entry_t entry) {
  /* allocated_memory mem = allocator_alloc(ht->allocator, sizeof(str_entry_t));
   */
  /* if (!mem.valid) { */
  /*   log_error("failed to allocate memory to store hash table entry\n"); */
  /*   return false; */
  /* } */

  str_entry_t *new_entry =
      allocator_alloc_or_exit(ht->allocator, sizeof(str_entry_t));
  memcpy(new_entry, &entry, sizeof(str_entry_t));

  uint32_t idx = str_hash(entry.key, ht->capacity);

  ht->entry_count = ht->entry_count + 1;
  if (ht->entries[idx] == NULL) {
    ht->entries[idx] = new_entry;
    return true;
  } else { // Handle collision
    log_info("Hash collision, adding item to list.\n");
    str_entry_t *existing_entry = ht->entries[idx];
    log_info("'{s}' collides with '{s}'\n", new_entry->key,
             existing_entry->key);
    while (existing_entry->next != NULL &&
           str_eq(existing_entry->key, new_entry->key) == false) {
      existing_entry = existing_entry->next;
    }
    if (str_eq(new_entry->key, existing_entry->key)) {
      log_info("Key already in hashtable, delete first if you want to replace "
               "it.\n");
      ht->entry_count -= 1;
      return false;
    } else {
      existing_entry->next = new_entry;
      return true;
    }
  }
}

// This hands back pointers to the keys in the hashtable.  Mutating the values
// of the keys will invalidate the hashtable.  i.e.  don't do it.
//
// This may be a bad idea, but I'm going with it for now.  Time will tell.
hashtbl_str_keys_t hashtbl_str_get_keys(hashtbl_str_t *ht) {
  assert(ht->valid);
  str **keys =
      allocator_alloc_or_exit(ht->allocator, sizeof(str *) * ht->entry_count);
  size_t count = 0;
  for (int i = 0; i < ht->capacity; i++) {
    if (ht->entries[i] != NULL) {
      str_entry_t *entry = ht->entries[i];
      keys[count++] = &entry->key;
      while (entry->next != NULL) {
        entry = entry->next;
        keys[count++] = &entry->key;
      }
    }
  }
  return (hashtbl_str_keys_t){.count = count, .keys = keys};
}

// Free the key memory
void hashtbl_str_free_keys(hashtbl_str_t ht, hashtbl_str_keys_t keys) {
  allocator_free(ht.allocator, keys.keys);
}

str_entry_t *hashtbl_str_lookup(hashtbl_str_t *ht, str key) {
  uint32_t idx = str_hash(key, ht->capacity);

  if (ht->entries[idx] != NULL) {
    str_entry_t *entry = ht->entries[idx];
    while (!str_eq(entry->key, key) && entry->next != NULL) {
      entry = entry->next;
    }
    if (str_eq(entry->key, key)) {
      return entry;
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

bool hashtbl_str_remove(hashtbl_str_t *ht, str key) {
  uint32_t idx = str_hash(key, ht->capacity);

  if (ht->entries[idx] != NULL) {
    str_entry_t *entry = ht->entries[idx];
    ht->entry_count -= 1;
    // Simple case, it's the first match
    if (str_eq(entry->key, key)) {
      if (entry->next != NULL) {
        ht->entries[idx] = entry->next;
      } else {
        ht->entries[idx] = NULL;
      }
      allocator_free(ht->allocator, entry);
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
      allocator_free(ht->allocator, next_entry);
      return true;
    }
  }
  return false;
}

hashtbl_str_t *hashtbl_str_init(allocator_t allocator) {
  hashtbl_str_t *self =
      allocator_alloc_or_exit(allocator, sizeof(hashtbl_str_t));
  self->allocator = allocator;
  self->entries = allocator_alloc_or_exit(allocator, sizeof(str_entry_t) *
                                                         DEFAULT_CAPACITY);
  self->entry_count = 0;
  self->valid = true;
  self->capacity = DEFAULT_CAPACITY;
  return self;
}
