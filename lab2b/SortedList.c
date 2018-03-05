// NAME: Christopher Aziz
// EMAIL: caziz@ucla.edu
//
//  SortedList.c
//  lab2a
//

#include "SortedList.h"
#include <stddef.h>
#include <string.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * SortedList (and SortedListElement)
 *
 *    A doubly linked list, kept sorted by a specified key.
 *    This structure is used for a list head, and each element
 *    of the list begins with this structure.
 *
 *    The list head is in the list, and an empty list contains
 *    only a list head.  The next pointer in the head points at
 *      the first (lowest valued) elment in the list.  The prev
 *      pointer in the list head points at the last (highest valued)
 *      element in the list.
 *
 *      The list head is also recognizable by its NULL key pointer.
 *
 */

int opt_yield;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    if (list == NULL || element == NULL) return;
    SortedListElement_t *curr = list->next;

    if (opt_yield & INSERT_YIELD)
        sched_yield();

    // find where element belongs
    for (; curr != list; curr=curr->next) {
        if (strcmp(element->key, curr->key) <= 0)
            break;
    }
    // insert element
    element->prev = curr->prev;
    element->next = curr;
    curr->prev->next = element;
    curr->prev = element;
}



int SortedList_delete( SortedListElement_t *element) {
    if (element->prev->next != element ||
        element->next->prev != element)
        return 1; // corrupted prev/next pointers

    if (opt_yield & DELETE_YIELD)
        sched_yield();

    // delete element
    element->next->prev = element->prev;
    element->prev->next = element->next;
    return 0;   // element deleted successfully
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    SortedListElement_t *curr = list->next;

    if (opt_yield & LOOKUP_YIELD)
        sched_yield();

    while (curr != list) {
        if (strcmp(curr->key, key) == 0)
            return curr;
        else
            curr = curr->next;
    }
    return NULL;   // couldn't find key}
}

int SortedList_length(SortedList_t *list) {
    SortedListElement_t *curr = list->next;
    int length = 0;
    if (opt_yield & LOOKUP_YIELD) {
        sched_yield();
    }
    for (; curr != list; curr = curr->next) {
        if (curr->prev->next != curr ||
            curr->next->prev != curr)
            return -1;
        length++;
    }
    return length;
}




