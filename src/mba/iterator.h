#ifndef MBA_ITERATOR_H
#define MBA_ITERATOR_H

/* iter - container for iterator state
 */

typedef struct _iter {
	unsigned long i1;
	unsigned long i2;
	unsigned long i3;
	void *p;
} iter_t;

typedef void (*iterate_fn)(void *obj, iter_t *iter);
typedef void *(*iterate_next_fn)(void *obj, iter_t *iter);

#endif /* MBA_ITERATOR_H */
