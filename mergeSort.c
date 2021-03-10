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

struct list_head {
    struct list_head *prev, *next;
};

typedef struct __element {
    char *value;
    struct __element *next;
    struct list_head list;
} list_ele_t;

typedef struct {
    list_ele_t *head; /* Linked list of elements */
    list_ele_t *tail;
    size_t size;
    struct list_head list;
} queue_t;


#define LIST_HEAD(head) struct list_head head = {&(head), &(head)}

static inline void INIT_LIST_HEAD(struct list_head *head)
{
    head->next = head; head->prev = head;
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

#define list_entry(node, type, member) container_of(node, type, member)

#define list_first_entry(head, type, member) \
    list_entry((head)->next, type, member)

#define list_for_each(node, head) \
    for (node = (head)->next; node != (head); node = node->next)

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
    INIT_LIST_HEAD(head);
    if (list_empty(lhs)) {
        list_splice_tail(lhs, head);
        return;
    }
    if (list_empty(rhs)) {
        list_splice_tail(rhs, head);
        return;
    }

    while (!list_empty(lhs) && !list_empty(rhs)) {
        char *lv = list_entry(lhs->next, list_ele_t, list)->value;
        char *rv = list_entry(rhs->next, list_ele_t, list)->value;
        struct list_head *tmp = strcmp(lv, rv) <= 0 ? lhs->next : rhs->next;
        list_del(tmp);
        list_add_tail(tmp, head);
    }
    list_splice_tail(list_empty(lhs) ? rhs : lhs, head);
}

void list_merge_sort(queue_t *q)
{
    if (list_is_singular(&q->list))
        return;

    queue_t left;
    struct list_head sorted;
    INIT_LIST_HEAD(&left.list);
    list_cut_position(&left.list, &q->list, &get_middle(&q->list)->list);
    list_merge_sort(&left);
    list_merge_sort(q);
    list_merge(&left.list, &q->list, &sorted);
    INIT_LIST_HEAD(&q->list);
    list_splice_tail(&sorted, &q->list);
}


/*
Using data from cities.txt validating
*/

static bool validate(queue_t *q)
{
    struct list_head *node;
    list_for_each (node, &q->list) {
        if (node->next == &q->list)
            break;
        if (strcmp(list_entry(node, list_ele_t, list)->value,
                   list_entry(node->next, list_ele_t, list)->value) > 0)
            return false;
    }
    return true;
}

static queue_t *q_new()
{
    queue_t *q = malloc(sizeof(queue_t));
    if (!q) return NULL;

    q->head = q->tail = NULL;
    q->size = 0;
    INIT_LIST_HEAD(&q->list);
    return q;
}

static void q_free(queue_t *q)
{
    if (!q) return;

    list_ele_t *current = q->head;
    while (current) {
        list_ele_t *tmp = current;
        current = current->next;
        free(tmp->value);
        free(tmp);
    }
    free(q);
}

bool q_insert_head(queue_t *q, char *s)
{
    if (!q) return false;

    list_ele_t *newh = malloc(sizeof(list_ele_t));
    if (!newh)
        return false;

    char *new_value = strdup(s);
    if (!new_value) {
        free(newh);
        return false;
    }

    newh->value = new_value;
    newh->next = q->head;
    q->head = newh;
    if (q->size == 0)
        q->tail = newh;
    q->size++;
    list_add_tail(&newh->list, &q->list);

    return true;
}

int main(void)
{
    FILE *fp = fopen("cities.txt", "r");
    if (!fp) {
        perror("failed to open cities.txt");
        exit(EXIT_FAILURE);
    }

    queue_t *q = q_new();
    char buf[256];
    while (fgets(buf, 256, fp))
        q_insert_head(q, buf);
    fclose(fp);

    list_merge_sort(q);
    assert(validate(q));

    q_free(q);

}
