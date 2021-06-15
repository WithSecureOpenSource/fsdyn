#include "list.h"

#include "fsalloc.h"
#include "fsdyn_version.h"
#include "list_imp.h"

list_t *make_list(void)
{
    list_t *list = fsalloc(sizeof *list);
    list->first = list->last = NULL;
    list->size = 0;
    return list;
}

void destroy_list(list_t *list)
{
    list_elem_t *element, *next;
    for (element = list->first; element != NULL; element = next) {
        next = element->next;
        fsfree(element);
    }
    fsfree(list);
}

const void *list_elem_get_value(list_elem_t *element)
{
    return element->value;
}

list_elem_t *list_get_first(list_t *list)
{
    return list->first;
}

list_elem_t *list_get_last(list_t *list)
{
    return list->last;
}

list_elem_t *list_get(list_t *list, const void *value)
{
    list_elem_t *element;
    for (element = list->first; element != NULL; element = element->next)
        if (element->value == value)
            return element;
    return NULL;
}

list_elem_t *list_get_by_index(list_t *list, int idx)
{
    int i;
    list_elem_t *element;
    for (element = list->first, i = 0; element != NULL && i != idx;
         element = element->next, i++)
        ;
    return element;
}

list_elem_t *list_next(list_elem_t *element)
{
    return element->next;
}

list_elem_t *list_previous(list_elem_t *element)
{
    return element->previous;
}

list_elem_t *list_append(list_t *list, const void *value)
{
    list_elem_t *element = fsalloc(sizeof *element);
    element->value = value;
    element->next = NULL;
    if (list->first == NULL) {
        element->previous = NULL;
        list->first = list->last = element;
    } else {
        element->previous = list->last;
        element->previous->next = element;
        list->last = element;
    }
    list->size++;
    return element;
}

list_elem_t *list_prepend(list_t *list, const void *value)
{
    list_elem_t *element = fsalloc(sizeof *element);
    element->value = value;
    element->previous = NULL;
    if (list->first == NULL) {
        element->next = NULL;
        list->first = list->last = element;
    } else {
        element->next = list->first;
        element->next->previous = element;
        list->first = element;
    }
    list->size++;
    return element;
}

list_elem_t *list_insert_before(list_t *list, const void *value,
                                list_elem_t *next)
{
    list_elem_t *element = fsalloc(sizeof *element);
    element->value = value;
    element->next = next;
    element->previous = next->previous;
    if (next->previous)
        next->previous->next = element;
    else
        list->first = element;
    next->previous = element;
    list->size++;
    return element;
}

void list_remove(list_t *list, list_elem_t *element)
{
    if (list->first == element)
        if (list->last == element)
            list->first = list->last = NULL;
        else {
            list->first = element->next;
            element->next->previous = NULL;
        }
    else if (list->last == element) {
        list->last = element->previous;
        element->previous->next = NULL;
    } else {
        element->next->previous = element->previous;
        element->previous->next = element->next;
    }
    fsfree(element);
    list->size--;
}

const void *list_pop_first(list_t *list)
{
    list_elem_t *element = list_get_first(list);
    if (element == NULL)
        return NULL;
    const void *value = list_elem_get_value(element);
    list_remove(list, element);
    return value;
}

const void *list_pop_last(list_t *list)
{
    list_elem_t *element = list_get_last(list);
    if (element == NULL)
        return NULL;
    const void *value = list_elem_get_value(element);
    list_remove(list, element);
    return value;
}

int list_empty(list_t *list)
{
    return list->first == NULL;
}

size_t list_size(list_t *list)
{
    return list->size;
}

void list_foreach(list_t *list, void (*f)(const void *value, void *arg),
                  void *arg)
{
    list_elem_t *e;
    for (e = list_get_first(list); e; e = list_next(e))
        f(list_elem_get_value(e), arg);
}

static void append_it(const void *value, void *copy)
{
    list_append(copy, value);
}

list_t *list_copy(list_t *list)
{
    list_t *copy = make_list();
    list_foreach(list, append_it, copy);
    return copy;
}
