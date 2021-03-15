#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#ifndef container_of
#define container_of(ptr, type, member)                            \
    __extension__({                                                \
        const __typeof__(((type *) 0)->member) *__pmember = (ptr); \
        (type *) ((char *) __pmember - offsetof(type, member));    \
    })
#endif

#define list_entry(node, type, member) container_of(node, type, member)

#define list_first_entry(head, type, member) \
    list_entry((head)->next, type, member)

#define list_for_each(node, head) \
    for (node = (head)->next; node != (head); node = node->next)


struct list_head {
    struct list_head *prev, *next;
    char *value;
};

typedef struct __element {
    char *value;
    struct list_head list;
} list_ele_t;

#define LIST_HEAD(head) struct list_head head = {&(head), &(head)}

static inline void INIT_LIST_HEAD(struct list_head *head)
{
    head->next = head; head->prev = head;
}

/*
Initialization of list_head
*/
static struct list_head *new_head()
{
    struct list_head *newh = malloc(sizeof(struct list_head));
    if (!newh)
        return NULL;
    INIT_LIST_HEAD(newh);
    return newh;
}


static inline void list_add_tail(struct list_head *node, struct list_head *head)
{
    struct list_head *prev = head->prev;

    prev->next = node;
    node->next = head;
    node->prev = prev;
    head->prev = node;
}

static inline void list_del(struct list_head *node)
{
    struct list_head *next = node->next, *prev = node->prev;
    next->prev = prev; prev->next = next;
}

static inline int list_empty(const struct list_head *head)
{
    return (head->next == head);
}

static inline int list_is_singular(const struct list_head *head)
{
    return (!list_empty(head) && head->prev == head->next);
}

static inline void list_splice_tail(struct list_head *list,
                                    struct list_head *head)
{
    struct list_head *head_last = head->prev;
    struct list_head *list_first = list->next, *list_last = list->prev;

    if (list_empty(list))
        return;

    head->prev = list_last;
    list_last->next = head;

    list_first->prev = head_last;
    head_last->next = list_first;
}

static inline void list_cut_position(struct list_head *head_to,
                                     struct list_head *head_from,
                                     struct list_head *node)
{
    struct list_head *head_from_first = head_from->next;

    if (list_empty(head_from))
        return;

    if (head_from == node) {
        INIT_LIST_HEAD(head_to);
        return;
    }

    head_from->next = node->next;
    head_from->next->prev = head_from;

    head_to->prev = node;
    node->next = head_to;
    head_to->next = head_from_first;
    head_to->next->prev = head_to;

}

static list_ele_t *get_middle(struct list_head *list)
{
    struct list_head *fast = list->next, *slow;
    list_for_each (slow, list) {
        if (fast->next == list || fast->next->next == list)
            break;
        fast = fast->next->next;
    }
    return list_entry(slow, list_ele_t, list);
}

static void list_merge(struct list_head *lhs,
                       struct list_head *rhs,
                       struct list_head *head)
{
    
    if (list_empty(lhs)) {
        list_splice_tail(lhs, head);
        return;
    }
    if (list_empty(rhs)) {
        list_splice_tail(rhs, head);
        return;
    }

    struct list_head *tmp_head = new_head();
    INIT_LIST_HEAD(tmp_head);
    while (!list_empty(lhs) && !list_empty(rhs)) {
        char *lv = list_entry(lhs->next, list_ele_t, list)->value;
        char *rv = list_entry(rhs->next, list_ele_t, list)->value;
        struct list_head *tmp = strcmp(lv, rv) <= 0 ? lhs->next : rhs->next;
        list_del(tmp);
        list_add_tail(tmp, tmp_head);
    }
    list_splice_tail(list_empty(lhs) ? rhs : lhs, tmp_head);
    INIT_LIST_HEAD(head);
    list_splice_tail(tmp_head, head);
}

void list_merge_sort(struct list_head *list_l)
{
    if (list_is_singular(list_l))
        return;

    struct list_head *list_r = new_head();
    list_r->value = "head";

    list_cut_position(list_r, list_l, &get_middle(list_l)->list);
    list_merge_sort(list_r);
    list_merge_sort(list_l);
    list_merge(list_l, list_r, list_l);
}

/*
Given a list_head *head, insert a new list_ele_t at the tail of list.
*/
bool insert_head(struct list_head *head, char *s)
{
    if (!head) return false;

    list_ele_t *newh = malloc(sizeof(list_ele_t));
    if (!newh)
        return false;

    char *new_value = strdup(s);
    if (!new_value) {
        free(newh);
        return false;
    }
    newh->value = new_value;
    newh->list.value = new_value;//test
    list_add_tail(&newh->list, head);

    return true;
}


/*
Using data from cities.txt validating
*/
static bool validate(struct list_head *head)
{
    struct list_head *node;
    list_for_each (node, head) {
        if (node->next == head)
            break;
        if (strcmp(list_entry(node, list_ele_t, list)->value,
                   list_entry(node->next, list_ele_t, list)->value) > 0)
            return false;
    }
    return true;
}

int main(void)
{
    FILE *fp = fopen("cities.txt", "r");
    if (!fp) {
        perror("failed to open cities.txt");
        exit(EXIT_FAILURE);
    }

    struct list_head *head = new_head();
    head->value = "head";
    char buf[256];
    while (fgets(buf, 256, fp))
        insert_head(head, buf);
    fclose(fp);


    list_merge_sort(head);

    assert(validate(head));

    
}
