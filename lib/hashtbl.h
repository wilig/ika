#pragma once

#include "../src/rt/str.h"
#include "allocator.h"

#define DEFAULT_CAPACITY 100

typedef struct str_entry_t {
  b8 valid;
  str key;
  void *value;
  struct str_entry_t *next;
} str_entry_t;

typedef struct hashtbl_str_keys_t {
  str **keys;
  u32 count;
} hashtbl_str_keys_t;

typedef struct hashtbl_str_t {
  u64 capacity;
  u64 entry_count;
  b8 valid;
  str_entry_t **entries;
} hashtbl_str_t;

hashtbl_str_t *hashtbl_str_init();
void hashtbl_str_deinit(hashtbl_str_t *);

hashtbl_str_keys_t hashtbl_str_get_keys(hashtbl_str_t *);
b8 hashtbl_str_insert(hashtbl_str_t *, str_entry_t);
str_entry_t *hashtbl_str_lookup(hashtbl_str_t *, str);
void hashtbl_str_delete(hashtbl_str_t *, str_entry_t);
