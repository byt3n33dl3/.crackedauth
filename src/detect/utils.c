#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

int
fd_accept(int fd, struct sockaddr * addr, socklen_t * addrlen)
{
	int ret;
	
	do {
		ret = accept(fd, addr, addrlen);
	} while (ret == -1 && errno == EINTR);
	if (ret < 0) pfatal("fd_accept");
	return ret;
}

void
fd_setnonblock(int fd)
{
	int flags;
	flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
}

void
fd_setblock(int fd)
{
	int flags;
	flags = fcntl(fd, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
}

void
fd_close(int fd)
{
	int ret;

	do {
		ret = close(fd);
	} while (ret == -1 && errno == EINTR);
	if (ret < 0) pfatal("fd_close");
}

size_t
fd_ravail(int fd)
{
	size_t avail;
	int ret;

	avail = 0;
	do {
		ret = ioctl(fd, FIONREAD, &avail);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));

	if (ret < 0) pfatal("fd_ravailable");
	return avail;
}

size_t
fd_read(int fd, void * buf, size_t size)
{
	void * p;
	size_t todo;
	int ret;

	p = buf;
	ret = 0;
	todo = size;
	do {
		if (ret < 0) ret = 0;
		p += ret;	
		if ((size_t)(p-buf) == size) break;
		todo -= ret;
		ret = read(fd, p, todo);
	} while (ret || ((ret < 0) && (errno == EINTR || errno == EAGAIN)));
	if (ret < 0) {
		pfatal("fd_read");
	}
	return (size_t)(p-buf);
}

size_t
fd_write(int fd, const void * buf, size_t size)
{
	const void * p;
	size_t todo;
	int ret;

	p = buf;
	ret = 0;
	todo = size;
	do {
		if (ret < 0) ret = 0;
		p += ret;	
		if ((size_t)(p-buf) == size) break;
		todo -= ret;
		ret = write(fd, p, todo);
	} while ((ret) || ((ret < 0) && (errno == EINTR || errno == EAGAIN)));
	if (ret < 0) pfatal("fd_write");
	return (size_t)(p-buf);
}

void
fd_set_cloexec(int fd)
{
	int flags;
	flags = fcntl(fd, F_GETFD);
	flags |= FD_CLOEXEC;
	fcntl(fd, F_SETFD, flags);
}

void *
xmalloc(size_t sz)
{
	void * ptr = malloc(sz);
	if (!ptr) pfatal("xmalloc");
	memset(ptr, 0, sz);
	return ptr;
}

char *
xstrdup(const char * s)
{
	size_t len;
	if (!s) pfatal("xstrdup");
	len = strlen(s);
	if (len > 0xffffff) fatal("input string too long");
	char * p = xmalloc(len+1);
	memcpy(p, s, strlen(s));
	p[len]=0;
	return p;
}

void *
xrealloc(void * p, size_t sz)
{
	void * ret;

	ret = realloc(p, sz);
	if (!ret) pfatal("xrealloc");
	return ret;
}

int
bin2hex(const char * src, size_t srclen, char * dst, size_t dstlen)
{
	const char * p;
	static char table[16]={'0','1','2','3','4','5','6','7','8','9',
		'A','B','C','D','E','F'};
	if (!src || !srclen || !dst) return -1;
	if (((srclen<<1) + 1) > dstlen || ((srclen<<1)+1) < srclen) return -1;

	p = src;
	do {
		*dst++ = table[((*p>>4) & 0x0f)];
		*dst++ = table[(*p & 0x0f)];
		p++;
	} while (--srclen);
	*dst = 0;

	return 0;
}

int
hex2bin(const char * src, size_t srclen, char * dst, size_t dstlen)
{
	int i;
	const char * p;
	char ch, ch2;

	if (!src || !srclen || !dst) return -1;
	if (srclen%2 || (srclen>>1 > dstlen)) return -1;

	p = src;
	do {
		ch2 = 0;
		for (i=4;i>=0;i-=4) {
			if (*p >= '0' && *p <= '9') ch = *p - '0';
			else if (*p >= 'a' && *p <= 'f') ch = *p - 'a' + 10; 
			else if (*p >= 'A' && *p <= 'F') ch = *p - 'A' + 10;
			else return -1;
			ch2 |= (ch << i);
			p++;
		}
		*dst = ch2;
		dst++;
		srclen -= 2;
	} while (srclen);

	return 0;
}

int
hex2bin_inplace(char * src, size_t srclen)
{
	return hex2bin(src, srclen, src, srclen>>1);
}

#ifdef DEBUG
#include <execinfo.h>

void
_backtrace()
{
	int j, nptrs;
	void *buffer[100];
	char **strings;

	nptrs = backtrace(buffer, 100);

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for (j = 0; j < nptrs; j++)
		fprintf(stderr, "%s\n", strings[j]);
	fflush(stderr);

	free(strings);
}
#endif

void
_fatal(const char * f, int l, const char * fmt, ...)
{
	char buf[1024];
	va_list ap;
#ifdef DEBUG
	_backtrace();
#endif
	memset(buf, 0, sizeof(buf));
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, ap);
	va_end(ap);
	if (f) fprintf(stderr, "%s:%i ", f, l);
	fprintf(stderr, "%s\n", buf);
	fflush(stderr);
	exit(EXIT_FAILURE);
}

void
_pfatal(const char * f, int l, const char * desc)
{
	const char * err;
#ifdef DEBUG
	_backtrace();
#endif
	err = strerror(errno);
	if (f) fprintf(stderr, "%s:%i in ", f, l);
	fprintf(stderr, "%s(): %s\n", desc, err);
	fflush(stderr);
	exit(EXIT_FAILURE);
}

#ifdef DEBUG
void
_debug(const char * f, int l, const char * fmt, ...)
{
	char buf[1024];
	va_list ap;
	memset(buf, 0, sizeof(buf));
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, ap);
	va_end(ap);
	
	fprintf(stderr, "%s:%i %s\n", f, l, buf);	
	fflush(stderr);
}
#endif
