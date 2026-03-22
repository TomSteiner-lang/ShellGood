#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define createArrayList(type) NewArrayList_##type()
#define DEF_ARRAYLIST(type)\
\
typedef struct {\
\
\
    type *array;\
    int length;\
    int capacity;\
\
\
\
} ArrayList_##type;\
\
\
\
static inline ArrayList_##type NewArrayList_##type(){\
    ArrayList_##type x;\
    x.length = 0;\
    x.capacity = 16;\
\
    x.array = (type *)malloc(sizeof(type) * x.capacity);\
    if (!x.array) {\
        x.capacity = 0;\
\
    };\
\
    return x;\
}\
\
\
static inline int ArrayListAdd_##type(type thing, ArrayList_##type *arr) {\
\
    if (arr->length >= arr->capacity) {\
        void *newArr = realloc(arr->array, arr->capacity * 2 * sizeof(type));\
        if (!newArr) {\
            return -1;\
        }\
\
        arr->array = (type *)newArr;\
        arr->capacity *= 2;\
    }\
\
    arr->array[arr->length] = thing;\
\
    arr->length++;\
    return arr->length-1;\
\
}\
\
static inline type ArrayListGet_##type(ArrayList_##type *arr, int index) {\
    return arr->array[index];\
}\
\
static inline void ArrayListKill_##type(ArrayList_##type *arr) {\
    free(arr->array);\
}\
\
static inline type ArrayListRemove_##type(int index, ArrayList_##type *arr) {\
    if (index >= arr->length || index < 0) {\
        abort();\
    }\
    type ret = arr->array[index];\
    int tail = arr->length - index - 1;\
    for (int i = 0; i < tail; i++) {\
        arr->array[index + i] = arr->array[index + i + 1];\
    }\
    memset(&arr->array[index+tail], 0, sizeof(type));\
    arr->length--;\
    \
    return ret;\
}
#endif



