#pragma once

#include <stdint.h>
#include "common/errno/errno.h"

struct List_ops;

typedef struct List_node {
  struct List_node *next;
  void *value;
} List_node;

typedef struct List {
  List_node *head;
  const struct List_ops *ops;
} List;

typedef uint8_t List_item_match(const void *const ctx, const void *const value);

struct List_ops {
  errno_t (*head_insert)(List *list, const void *value);
  errno_t (*list_remove_node)(List *list, List_node *node);
  errno_t (*find)(const List *list, void *return_value_ptr, const void *const ctx, List_item_match *const match);
};

errno_t list_create(List **new_list_ptr);
