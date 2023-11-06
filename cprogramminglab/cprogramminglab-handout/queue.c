/**
 * @file queue.c
 * @brief Implementation of a queue that supports FIFO and LIFO operations.
 *
 * This queue implementation uses a singly-linked list to represent the
 * queue elements. Each queue element stores a string value.
 *
 * Assignment for basic C skills diagnostic.
 * Developed for courses 15-213/18-213/15-513 by R. E. Bryant, 2017
 * Extended to store strings, 2018
 *
 * TODO: fill in your name and Andrew ID
 * @author XXX <XXX@andrew.cmu.edu>
 */

#include "queue.h"
#include "harness.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Allocates a new queue
 * @return The new queue, or NULL if memory allocation failed
 */
queue_t *queue_new(void) {
    queue_t *q = (queue_t *)malloc(sizeof(queue_t));
    if (q == NULL)
        return NULL;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

char *strdup(const char *str) {
    char *p = (char *)malloc(strlen(str) + 1);
    if (p == NULL)
        return NULL;
    strcpy(p, str);
    return p;
}
/**
 * @brief Frees all memory used by a queue
 * @param[in] q The queue to free
 */
void queue_free(queue_t *q) {
    /* How about freeing the list elements and the strings? */
    /* Free queue structure */
    if (q == NULL)
        return;
    list_ele_t *p = q->head;
    list_ele_t *del;
    while (p) {
        del = p;
        p = p->next;
        free(del->value);
        free(del);
    }
    free(q);
}

/**
 * @brief Attempts to insert an element at head of a queue
 *
 * This function explicitly allocates space to create a copy of `s`.
 * The inserted element points to a copy of `s`, instead of `s` itself.
 *
 * @param[in] q The queue to insert into
 * @param[in] s String to be copied and inserted into the queue
 *
 * @return true if insertion was successful
 * @return false if q is NULL, or memory allocation failed
 */
bool queue_insert_head(queue_t *q, const char *s) {
    if (q == NULL)
        return false;
    char *str = strdup(s);
    if (str == NULL)
        return false;
    list_ele_t *newh = new_ele(str);
    if (newh == NULL) {
        free(str);
        return false;
    }
    if (q->head == NULL) {
        q->head = q->tail = newh;
    } else {
        newh->next = q->head;
        q->head = newh;
    }
    q->size++;
    return true;
}

/**
 * @brief Attempts to insert an element at tail of a queue
 *
 * This function explicitly allocates space to create a copy of `s`.
 * The inserted element points to a copy of `s`, instead of `s` itself.
 *
 * @param[in] q The queue to insert into
 * @param[in] s String to be copied and inserted into the queue
 *
 * @return true if insertion was successful
 * @return false if q is NULL, or memory allocation failed
 */
bool queue_insert_tail(queue_t *q, const char *s) {
    if (q == NULL)
        return false;
    char *str = strdup(s);
    if (str == NULL)
        return false;
    list_ele_t *newt = new_ele(str);
    if (newt == NULL) {
        free(str); // don't forget to free this string!!!
        return false;
    }
    if (q->tail == NULL) {
        q->head = q->tail = newt;
    } else {
        q->tail->next = newt;
        q->tail = newt;
    }
    q->size++;
    return true;
}

size_t mymin(size_t a, size_t b) {
    return a < b ? a : b;
}
/**
 * @brief Attempts to remove an element from head of a queue
 *
 * If removal succeeds, this function frees all memory used by the
 * removed list element and its string value before returning.
 *
 * If removal succeeds and `buf` is non-NULL, this function copies up to
 * `bufsize - 1` characters from the removed string into `buf`, and writes
 * a null terminator '\0' after the copied string.
 *
 * @param[in]  q       The queue to remove from
 * @param[out] buf     Output buffer to write a string value into
 * @param[in]  bufsize Size of the buffer `buf` points to
 *
 * @return true if removal succeeded
 * @return false if q is NULL or empty
 */
bool queue_remove_head(queue_t *q, char *buf, size_t bufsize) {
    /* You need to fix up this code. */
    if (q == NULL || q->head == NULL)
        return false;
    list_ele_t *h = q->head;
    q->head = q->head->next;
    if (buf != NULL) {
        size_t minsize = mymin(bufsize - 1, strlen(h->value));
        strncpy(buf, h->value, minsize);
        buf[minsize] = '\0';
    }
    free(h->value);
    free(h);
    q->size--;
    return true;
}

/**
 * @brief Returns the number of elements in a queue
 *
 * This function runs in O(1) time.
 *
 * @param[in] q The queue to examine
 *
 * @return the number of elements in the queue, or
 *         0 if q is NULL or empty
 */
size_t queue_size(queue_t *q) {
    /* You need to write the code for this function */
    /* Remember: It should operate in O(1) time */
    if (q == NULL)
        return 0;
    return q->size;
}

/**
 * @brief Reverse the elements in a queue
 *
 * This function does not allocate or free any list elements, i.e. it does
 * not call malloc or free, including inside helper functions. Instead, it
 * rearranges the existing elements of the queue.
 *
 * @param[in] q The queue to reverse
 */
void queue_reverse(queue_t *q) {
    /* You need to write the code for this function */
    if (q == NULL || q->head == NULL || q->head->next == NULL)
        return;
    list_ele_t *cur = q->head, *next = q->head->next;
    list_ele_t *newtail = cur;
    cur->next = NULL;
    while (next != q->tail) {
        list_ele_t *newnext = next->next;
        next->next = cur;
        cur = next;
        next = newnext;
    }
    next->next = cur;
    q->head = next;
    q->tail = newtail;
}

list_ele_t *new_ele(char *val) {
    list_ele_t *ele = (list_ele_t *)malloc(sizeof(list_ele_t));
    if (ele == NULL)
        return NULL;
    ele->next = NULL;
    ele->value = val;
    return ele;
}
