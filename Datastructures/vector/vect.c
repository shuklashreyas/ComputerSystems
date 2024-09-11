/**
 * Vector implementation.
 *
 * - Implement each of the functions to create a working growable array (vector).
 * - Do not change any of the structs
 * - When submitting, You should not have any 'printf' statements in your vector 
 *   functions.
 *
 * IMPORTANT: The initial capacity and the vector's growth factor should be 
 * expressed in terms of the configuration constants in vect.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "vect.h"

/** Main data structure for the vector. */
struct vect {
  char **data;
  unsigned int size;
  unsigned int capacity;
};

char *my_strdup(const char *s) {
    if (!s) return NULL;
    char *d = malloc(strlen(s) + 1);
    if (d) {
        strcpy(d, s);
    }
    return d;
}
/** Construct a new empty vector. */
vect_t *vect_new() {
  vect_t *v = malloc(sizeof(vect_t));
  if (!v) return NULL;

  v->data = (char **)malloc(VECT_INITIAL_CAPACITY * sizeof(char *));
  if (!v->data) {
    free(v);
    return NULL;
  }

  v->size = 0;
  v->capacity = VECT_INITIAL_CAPACITY;
  return v;
}

/** Delete the vector, freeing all memory it occupies. */
void vect_delete(vect_t *v) {
    for(unsigned int i = 0; i < v->size; i++) {
        free(v->data[i]);
    }
    free(v->data);
    free(v);
}

/** Get the element at the given index. */
const char *vect_get(vect_t *v, unsigned int idx) {
    return v->data[idx];
}

/** Get a copy of the element at the given index. The caller is responsible
 *  for freeing the memory occupied by the copy. */
char *vect_get_copy(vect_t *v, unsigned int idx) {
    return my_strdup(v->data[idx]);
}

/** Set the element at the given index. */
void vect_set(vect_t *v, unsigned int idx, const char *elt) {
    char *new_elt = my_strdup(elt);
    if (new_elt) {
        free(v->data[idx]);
        v->data[idx] = new_elt;
    }
}

/** Add an element to the back of the vector. */
void vect_add(vect_t *v, const char *elt) {
    if (v->size == v->capacity) {
        unsigned int new_capacity = v->capacity * VECT_GROWTH_FACTOR;
        char **new_data = (char **)realloc(v->data, new_capacity * sizeof(char *));
        if (!new_data) return;
        v->data = new_data;
        v->capacity = new_capacity;
    }
    v->data[v->size++] = my_strdup(elt);
}

/** Remove the last element from the vector. */
void vect_remove_last(vect_t *v) {
    if (v->size > 0) {
        free(v->data[--v->size]);
    }
}

/** The number of items currently in the vector. */
unsigned int vect_size(vect_t *v) {
    return v->size;
}

/** The maximum number of items the vector can hold before it has to grow. */
unsigned int vect_current_capacity(vect_t *v) {
    return v->capacity;
}
