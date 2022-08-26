#pragma once

#include "allocator.h"
#include "tokenize.h"

#define DEFAULT_CAPACITY 100

typedef struct str_entry_t {
  bool valid;
  str key;
  void *value;
  struct str_entry_t *next;
} str_entry_t;

typedef struct hashtbl_str_t {
  allocator_t allocator;
  uint64_t capacity;
  bool valid;
  str_entry_t **entries;
} hashtbl_str_t;

hashtbl_str_t hashtbl_str_init(allocator_t);
void hashtbl_str_deinit(hashtbl_str_t);

bool hashtbl_str_insert(hashtbl_str_t, str_entry_t);
str_entry_t hashtbl_str_lookup(hashtbl_str_t, str);
void hashtbl_str_delete(hashtbl_str_t, str_entry_t);