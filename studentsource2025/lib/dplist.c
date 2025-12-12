

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"




/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;

    void *(*element_copy)(void *src_element);

    void (*element_free)(void **element);

    int (*element_compare)(void *x, void *y);
};


dplist_t *dpl_create(// callback functions
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {
    if (list == NULL || *list == NULL) return;

    dplist_node_t *dummy = (*list)->head;
    while (dummy != NULL)
    {
        dplist_node_t *next_dummy = dummy->next;
        if (free_element) (*list)->element_free(&dummy->element);
        free(dummy);
        dummy = next_dummy;
    }
    free(*list);
    *list = NULL;

}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy)
{
    dplist_node_t *ref_at_index, *list_node;
    if (list == NULL) return NULL;

    list_node = malloc(sizeof(dplist_node_t));
    if (insert_copy)
    {
        list_node->element = list->element_copy(element);
    } else
    {
        list_node->element = element;
    }


    // pointer drawing breakpoint
    if (list->head == NULL) { // covers case 1
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
        // pointer drawing breakpoint
    } else if (index <= 0) { // covers case 2
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
        // pointer drawing breakpoint
    } else {
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);
        // pointer drawing breakpoint
        if (index < dpl_size(list)) { // covers case 4
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;
            // pointer drawing breakpoint
        } else { // covers case 3
            assert(ref_at_index->next == NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
            // pointer drawing breakpoint
        }
    }
    //free(list_node->element);
    return list;


}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
    if (list == NULL || list->head == NULL) return NULL;

    dplist_node_t* ref_at_index = dpl_get_reference_at_index(list, index);
    if (ref_at_index == NULL) return list;

    dplist_node_t* list_prev = ref_at_index->prev;
    dplist_node_t* list_next = ref_at_index->next;

    if (list_prev != NULL)
    {
        list_prev->next = list_next;
    }else
    {
        list->head = list_next;
    }

    if (list_next != NULL)
    {
        list_next->prev = list_prev;
    }
    if (free_element)   list->element_free(&ref_at_index->element);
    free(ref_at_index);

    return list;


}

int dpl_size(dplist_t *list) {
    if (list == NULL) return -1;
    dplist_node_t *dummy = list->head;
    int size = 0;
    while (dummy!= NULL)
    {
        size++;
        dummy = dummy->next;
    }

    return size;

}

void *dpl_get_element_at_index(dplist_t *list, int index) {
    dplist_node_t *reference = dpl_get_reference_at_index(list, index);
    if (reference == NULL) return 0;
    return reference->element;

}

int dpl_get_index_of_element(dplist_t *list, void *element) {
    dplist_node_t *dummy = list->head;
    if (list == NULL || list->head == NULL) return -1;
    int index = 0;
    while (dummy != NULL)
    {
        if (list->element_compare(dummy->element, element) == 0)
        {
            return index;
        }
        dummy = dummy->next;
        index++;
    }
    return -1;

}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) return NULL;
    dplist_node_t *dummy = NULL;
    if(index <=0){
        dummy = list->head;
    } else if(index < dpl_size(list)){
        dummy = list->head;
        int i = 1;
        while (i <= index)
        {
            dummy = dummy->next;
            i++;
        }
    } else
    {
        dummy = list->head;
        while(dummy->next != NULL)
        {
            dummy = dummy->next;
        }
    }
    return dummy;

}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
    if (list == NULL || list->head == NULL || reference == NULL) return NULL;
    return reference->element;
}


