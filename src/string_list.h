#ifndef STRING_LIST_H
#define STRING_LIST_H
#include "types.h"

struct string_list_node {
    struct string_list_node * next;
    struct string_list_node * prev;
    char * str;
};

struct string_list {
    struct string_list_node * head;
    struct string_list_node * tail;
};

void string_list_push(struct string_list * list, const char * str);
void string_list_pop(struct string_list * list);
char* string_list_pop_str(struct string_list * list);
void string_list_push_pop(struct string_list * to, struct string_list * from);
void string_list_shift(struct string_list * list);
char* string_list_shift_str(struct string_list * list);
void string_list_clear(struct string_list * list);
int string_split_path(const char * path, struct string_list * pathc, int addExt);

#endif
