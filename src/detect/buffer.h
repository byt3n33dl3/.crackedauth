#ifndef BUFFER_H
  #define BUFFER_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#define INITIAL_BUF_SIZE	256

struct buffer {
	void * data;
	size_t length;
	size_t roff;
	size_t woff;
};

struct buffer * buffer_new();
void buffer_free(struct buffer *);
void buffer_expand(struct buffer *, size_t);
void buffer_append(struct buffer *, void *, size_t);
void buffer_prepend(struct buffer *, void *, size_t);
void buffer_peek(struct buffer *, void *, size_t);
int buffer_peek_until(struct buffer *, void *, size_t, char, size_t *);
void buffer_consume(struct buffer *, void *, size_t);
int buffer_consume_until(struct buffer *, void *, size_t, char, size_t *);
int buffer_findchar(struct buffer *, int, size_t *);
size_t buffer_avail(struct buffer *);
void buffer_reset(struct buffer *);
void buffer_fd_append(struct buffer *, int, size_t);
void buffer_fd_write(struct buffer *, int, size_t);
void buffer_fd_flush(struct buffer *, int);

#endif
