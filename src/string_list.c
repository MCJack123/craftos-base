#include "string_list.h"
#include <string.h>

static int allblank(const char * str) {
    while (*str) {
        if (*str != ' ' && *str != '.') return 0;
        str++;
    }
    return 1;
}

static int strall(const char * str, char c) {
    while (*str) if (*str++ != c) return 0;
    return 1;
}

void string_list_push(struct string_list * list, const char * str) {
    if (list->head == NULL) {
        list->head = list->tail = F.malloc(sizeof(struct string_list_node));
        list->head->prev = NULL;
    } else {
        list->tail->next = F.malloc(sizeof(struct string_list_node));
        list->tail->next->prev = list->tail;
        list->tail = list->tail->next;
    }
    list->tail->next = NULL;
    list->tail->str = F.malloc(strlen(str) + 1);
    strcpy(list->tail->str, str);
}

void string_list_pop(struct string_list * list) {
    struct string_list_node * next;
    if (!list->head) return;
    next = list->tail->prev;
    F.free(list->tail->str);
    F.free(list->tail);
    if ((list->tail = next) == NULL) list->head = NULL;
}

char* string_list_pop_str(struct string_list * list) {
    struct string_list_node * next;
    char* retval;
    if (!list->head) return;
    next = list->tail->prev;
    retval = list->tail->str;
    F.free(list->tail);
    if ((list->tail = next) == NULL) list->head = NULL;
}

void string_list_push_pop(struct string_list * to, struct string_list * from) {
    struct string_list_node * next;
    if (!from->tail) return;
    from->tail->prev->next = NULL;
    if (to->head == NULL) {
        to->head = to->tail = from->tail;
        from->tail = from->tail->prev;
        to->head->prev = NULL;
    } else {
        to->tail->next = from->tail;
        from->tail = from->tail->prev;
        to->tail->next->prev = to->tail;
        to->tail = to->tail->next;
    }
    if (from->tail == NULL) from->head = NULL;
}

void string_list_shift(struct string_list * list) {
    struct string_list_node * next;
    if (!list->head) return;
    next = list->head->next;
    F.free(list->head->str);
    F.free(list->head);
    if ((list->head = next) == NULL) list->tail = NULL;
}

char* string_list_shift_str(struct string_list * list) {
    struct string_list_node * next;
    char* str;
    if (!list->head) return NULL;
    next = list->head->next;
    str = list->head->str;
    F.free(list->head);
    if ((list->head = next) == NULL) list->tail = NULL;
    return str;
}

void string_list_clear(struct string_list * list) {
    struct string_list_node * next;
    while (list->head) {
        next = list->head->next;
        F.free(list->head->str);
        F.free(list->head);
        list->head = next;
    }
    list->tail = NULL;
}

int string_split_path(const char * path, struct string_list * pathc, int addExt) {
    char * s, * s2;
    char * pathfix = F.malloc(strlen(path) + 1);
    for (s = pathfix; *path; path++) {
        char c = *path;
        if (!(c == '"' || c == '*' || c == ':' || c == '<' || c == '>' || c == '?' || c == '|' || c < 32)) *s++ = c;
    }
    *s = 0;
    for (s = strtok(pathfix, "/\\"); s; s = strtok(NULL, "/\\")) {
        if (strncmp(s, "..", 2) == 0 && strall(s + 2, ' ')) {
            if (pathc->head == NULL && addExt) {
                F.free(pathfix);
                string_list_clear(pathc);
                return -1;
            } else if (pathc->head == NULL) string_list_push(pathc, "..");
            else string_list_pop(pathc);
        } else if (*s != 0 && !allblank(s)) {
            while (*s == ' ') s++;
            s2 = s;
            while (*s2++);
            s2--; s2--;
            while (*s2 == ' ' && s2 > s) s2--;
            *(s2 + 1) = 0;
            string_list_push(pathc, s);
        }
    }
    F.free(pathfix);
    return 0;
}
