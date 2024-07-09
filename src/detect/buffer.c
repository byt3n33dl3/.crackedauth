#include <string.h>

#include "buffer.h"
#include "utils.h"

struct buffer *
buffer_new()
{
	struct buffer * p;
	p = xmalloc(sizeof(struct buffer));
	p->data = xmalloc(INITIAL_BUF_SIZE);
	p->length = INITIAL_BUF_SIZE;
	p->roff = p->woff = 0;
	return p;
}

void
buffer_free(struct buffer * b)
{
	if (!b) fatal("buffer_free");
	free(b->data);
	free(b);
}

void
buffer_expand(struct buffer * b, size_t len)
{
	if (!b) fatal("buffer_expand");
	if ((b->woff + len) > b->length) {
		b->data = xrealloc(b->data, b->woff + len);
	}
	b->length = b->woff + len;
}

void
buffer_append(struct buffer * b, void * p, size_t len)
{
	if (!b) fatal("buffer_append");
	buffer_expand(b, len);
	if (p) {
		memcpy(b->data + b->woff, p, len);
		b->woff += len;
	}
}

void
buffer_prepend(struct buffer * b, void * p, size_t len)
{
	if (!b || !p || !len) fatal("buffer_prepend");
	if (len < b->roff) {
		b->roff -= len;
		memcpy(b->data + b->roff, p, len);
	}
	else {
		buffer_expand(b, len + buffer_avail(b));
		memmove(b->data + len, b->data + b->roff, b->woff - b->roff);
		memcpy(b->data, p, len);	
		b->woff += (len-b->roff);
		b->roff = 0;
	}
}

size_t
buffer_avail(struct buffer * b)
{
	if (!b) fatal("buffer_avail");
	return b->woff - b->roff;
}

void
buffer_peek(struct buffer * b, void * p, size_t len)
{
	if (!b || !p || len > buffer_avail(b)) fatal("buffer_peek");
	memcpy(p, b->data + b->roff, len);
}

void
buffer_consume(struct buffer * b, void * p, size_t len)
{
	if (!b) fatal("buffer_consume");
	if (p) buffer_peek(b, p, len);
	b->roff += len;
}

int
buffer_peek_until(struct buffer * b, void * p, size_t len, char c,
	size_t * consumed)
{
	size_t roff;
	char * p2;
	if (!b) fatal("buffer_consume_until");

	roff = b->roff;
	while (roff < b->woff && (!p || len-- > 0)) {
		p2 = b->data + roff++;
		if (p) *(char *)p++ = *p2;
		if (*p2 == c) {
			if (consumed)
				*consumed = roff - b->roff;
			return 1;
		}
	}
	if (consumed)
		*consumed = roff - b->roff;
	return 0;
}

int
buffer_consume_until(struct buffer * b, void * p, size_t len, char c,
	size_t * consumed)
{
	size_t tmpconsumed;
	int ret;

	if (!b) fatal("buffer_consume_until");

	ret = buffer_peek_until(b, p, len, c, &tmpconsumed);
	b->roff += tmpconsumed;
	if (consumed) *consumed = tmpconsumed;
	return ret;
}


int
buffer_findchar(struct buffer * b, int chr, size_t * off)
{
	void * ret;
	if (!b || !off) fatal("buffer_find");
	ret = memchr(b->data + b->roff, chr, b->woff - b->roff);
	if (!ret) return -1;	
	*off = ret - b->data - b->roff;	
	return 0;
}

void
buffer_reset(struct buffer * b)
{
	if (!b) fatal("buffer_reset");
	b->roff = b->woff = 0;
}

void
buffer_fd_append(struct buffer * b, int fd, size_t len)
{
	if (!b) fatal("buffer_fd_append");
	buffer_expand(b, len);
	fd_read(fd, b->data + b->woff, len);
	b->woff += len;
}

void
buffer_fd_write(struct buffer * b, int fd, size_t len)
{
	if (!b || len > (b->woff - b->roff)) fatal("buffer_fd_write");
	fd_write(fd, b->data + b->roff, len);
	b->roff += len;
}

void
buffer_fd_flush(struct buffer * b, int fd)
{
	if (!b) fatal("buffer_fd_flush");
	fd_write(fd, b->data + b->roff, b->woff - b->roff);
	b->roff = b->woff;
}
