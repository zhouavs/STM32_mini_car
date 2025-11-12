#include "list.h"
#include <stdlib.h>

static errno_t list_head_insert(List *list, const void *value);
static errno_t list_find(const List *list, void *return_value_ptr, const void *const ctx, List_item_match *const match);

static const struct List_ops ops = {
  .head_insert = list_head_insert,
  .find = list_find,
};

errno_t list_create(List **new_list_ptr) {
  if (new_list_ptr == NULL) return EINVAL;

  List *p = (List *)malloc(sizeof(List));
  if (p == NULL) return ENOMEM;

  p->head = NULL;
  p->ops = &ops;

  *new_list_ptr = p;
  return ESUCCESS;
}

static errno_t list_head_insert(List *list, const void *value) {
  if (list == NULL) return EINVAL;

  List_node *pnode = (List_node *)malloc(sizeof(List_node));
  if (pnode == NULL) return ENOMEM;
  // Store the pointer; list does not mutate the pointee
  pnode->value = (void *)value;
  pnode->next = NULL;

  if (list->head == NULL) {
    list->head = pnode;
    return ESUCCESS;
  }

  pnode->next = list->head;
  list->head = pnode;
  return ESUCCESS;
}

static errno_t list_find(const List *list, void *return_value_ptr, const void *const ctx, List_item_match *const match) {
  if (list == NULL || return_value_ptr == NULL || match == NULL) return EINVAL;

  List_node *pn = list->head;

  while (pn != NULL) {
    if (match(ctx, pn->value)) {
      *(void **)return_value_ptr = pn->value;
      return ESUCCESS;
    }

    pn = pn->next;
  }

  return E_CUSTOM_ITEM_NOT_FOUND;
}
