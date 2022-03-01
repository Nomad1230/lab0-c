#include <linux/kernel.h>
#include <linux/string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "harness.h"
#include "queue.h"
typedef unsigned char u8;
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
typedef int
    __attribute__((nonnull(2, 3))) (*list_cmp_func_t)(void *,
                                                      const struct list_head *,
                                                      const struct list_head *);

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    struct list_head *new = malloc(sizeof(struct list_head));
    // check if malloc success
    if (!new)
        return NULL;
    else {
        INIT_LIST_HEAD(new);
        return new;
    }
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;
    struct list_head *node, *safe;
    list_for_each_safe (node, safe, l) {
        list_del_init(node);
        q_release_element(list_entry(node, element_t, list));
    }
    free(l);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head || !s)
        return false;
    element_t *new = malloc(sizeof(element_t));
    if (!new)
        return false;
    else {
        new->value = strdup(s);
        if (!new->value) {
            free(new);
            return false;
        } else {
            list_add(&new->list, head);
            return true;
        }
    }
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head || !s)
        return false;
    element_t *new = malloc(sizeof(element_t));
    if (!new)
        return false;
    else {
        new->value = strdup(s);
        if (!new->value) {
            free(new);
            return false;
        } else {
            list_add_tail(&new->list, head);
            return true;
        }
    }
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || head->next == head)
        return NULL;
    else {
        struct list_head *del = head->next;
        list_del_init(head->next);
        element_t *del_ele = list_entry(del, element_t, list);
        if (sp) {
            strncpy(sp, del_ele->value, bufsize - 1);
            *(sp + bufsize - 1) = '\0';
        }
        return del_ele;
    }
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || head->next == head)
        return NULL;
    else {
        struct list_head *del = head->prev;
        list_del_init(head->prev);
        element_t *del_ele = list_entry(del, element_t, list);
        if (sp) {
            strncpy(sp, del_ele->value, bufsize - 1);
            *(sp + bufsize - 1) = '\0';
        }
        return del_ele;
    }
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head || head->next == head)
        return 0;
    int count = 0;
    struct list_head *node;
    list_for_each (node, head) {
        count++;
    }
    return count;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || head->next == head)
        return false;
    element_t *del;
    struct list_head **indir = &head;
    struct list_head *temp = head->prev;
    temp->next = NULL;
    for (struct list_head *fast = head; fast && fast->next;
         fast = fast->next->next)
        indir = &(*indir)->next;
    temp->next = head;
    del = list_entry(*indir, element_t, list);
    list_del_init(*indir);
    q_release_element(del);
    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head)
        return false;
    element_t *node, *safe;
    list_for_each_entry_safe (node, safe, head, list) {
        // if &safe->list == head, safe->value will dereference a null pointer
        if (&safe->list != head) {
            if (node->value && safe->value) {
                if (!strcmp(node->value, safe->value)) {
                    list_del(&node->list);
                    q_release_element(node);
                }
            }
        }
    }
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head || head->next == head || head->next->next == head)
        return;
    for (struct list_head *node1 = head->next, *node2 = head->next->next;
         node1 != head && node2 != head;
         node1 = node1->next, node2 = node1->next) {
        node1->prev->next = node2;
        node2->prev = node1->prev;
        node1->prev = node2;
        node1->next = node2->next;
        node2->next->prev = node1;
        node2->next = node1;
    }
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head || head->next == head)
        return;
    struct list_head *node = head, *temp;
    head->prev->next = NULL;
    head->prev = NULL;
    while (1) {
        temp = node->next;
        node->next = node->prev;
        node->prev = temp;
        if (!node->prev)
            break;
        else
            node = node->prev;
    }
    node->prev = head;
    head->next = node;
}

struct list_head *mergeTwoLists(struct list_head *L1, struct list_head *L2)
{
    struct list_head *head = NULL, **ptr = &head, **node;

    for (node = NULL; L1 && L2; *node = (*node)->next) {
        node = (strcmp(list_entry(L1, element_t, list)->value,
                       list_entry(L2, element_t, list)->value) < 0)
                   ? &L1
                   : &L2;
        *ptr = *node;
        ptr = &(*ptr)->next;
    }
    *ptr = (struct list_head *) ((u_int64_t) L1 | (u_int64_t) L2);
    return head;
}

static struct list_head *mergesort(struct list_head *head)
{
    if (!head || !head->next)
        return head;

    struct list_head *slow = head, *fast = head;
    while (fast && fast->next) {
        fast = fast->next->next;
        slow = slow->next;
    }

    slow->prev->next = NULL;

    struct list_head *left = mergesort(head);
    struct list_head *right = mergesort(slow);
    return mergeTwoLists(left, right);
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head)
{
    if (!head || head->next == head || head->next->next == head)
        return;
    head->prev->next = NULL;
    head->next = mergesort(head->next);
    struct list_head *node = head->next;
    node->prev = head;
    while (node->next) {
        node->next->prev = node;
        node = node->next;
    }
    node->next = head;
    head->prev = node;
}



static struct list_head *merge(void *priv,
                               struct list_head *a,
                               struct list_head *b)
{
    struct list_head *head = NULL, **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (strcmp(list_entry(a, element_t, list)->value,
                   list_entry(b, element_t, list)->value) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}



static void merge_final(void *priv,
                        struct list_head *head,
                        struct list_head *a,
                        struct list_head *b)
{
    struct list_head *tail = head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (strcmp(list_entry(a, element_t, list)->value,
                   list_entry(b, element_t, list)->value) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* Finish linking remainder of list b on to tail */
    tail->next = b;
    do {
        /*
         * If the merge is highly unbalanced (e.g. the input is
         * already sorted), this loop may run many iterations.
         * Continue callbacks to the client even though no
         * element comparison is needed, so the client's cmp()
         * routine can invoke cond_resched() periodically.
         */
        /*if (unlikely(!++count))
            strcmp(list_entry(a, element_t, list)->value,
                   list_entry(b, element_t, list)->value);*/
        b->prev = tail;
        tail = b;
        b = b->next;
    } while (b);

    /* And the final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}



void list_sort(void *priv, struct list_head *head)
{
    struct list_head *list = head->next, *pending = NULL;
    size_t count = 0; /* Count of pending */

    if (list == head->prev) /* Zero or one elements */
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;
    do {
        size_t bits;
        struct list_head **tail = &pending;

        /* Find the least-significant clear bit in count */
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        /* Do the indicated merge */
        if (likely(bits)) {
            struct list_head *a = *tail, *b = a->prev;

            a = merge(priv, b, a);
            /* Install the merged result in place of the inputs */
            a->prev = b->prev;
            *tail = a;
        }

        /* Move one element from input list to pending */
        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        count++;
    } while (list);
    /* End of input; merge together all the pending lists. */
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;

        if (!next)
            break;
        list = merge(priv, pending, list);
        pending = next;
    }
    /* The final merge, rebuilding prev links */
    merge_final(priv, head, pending, list);
}



void q_linuxsort(struct list_head *head)
{
    list_sort(0, head);
}

void q_shuffle(struct list_head *head)
{
    struct list_head *select = head;
    for (int size = q_size(head), rnd; size > 0; size--) {
        rnd = rand() % size + 1;
        do {
            select = select->next;
            rnd--;
        } while (rnd > 0);
        list_del_init(select);
        list_add_tail(select, head);
        select = head;
    }
}