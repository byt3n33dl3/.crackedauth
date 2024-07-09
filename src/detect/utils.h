#ifndef UTILS_H
  #define UTILS_H

#include <sys/types.h>
#include <sys/socket.h>

int fd_accept(int, struct sockaddr *, socklen_t *);
void fd_setnonblock(int);
void fd_setblock(int);
void fd_close(int);
size_t fd_read(int, void *, size_t);
size_t fd_write(int, const void *, size_t);
size_t fd_ravail(int);
void fd_set_cloexec(int);

void * xmalloc(size_t);
void * xrealloc(void *, size_t);
char * xstrdup(const char *);

int bin2hex(const char *, size_t, char *, size_t);
int hex2bin(const char *, size_t, char *, size_t);
int hex2bin_inplace(char *, size_t);

void _fatal(const char *, int, const char *, ...);
void _pfatal(const char *, int, const char *);

#ifdef DEBUG
void _debug(const char *, int, const char *, ...);
#define debug(...) _debug(__FILE__, __LINE__, __VA_ARGS__);
#define fatal(...) _fatal(__FILE__, __LINE__, __VA_ARGS__);
#define pfatal(x) _pfatal(__FILE__, __LINE__, x);
#else
#define debug(...) do{}while(0);
#define fatal(...) _fatal(NULL, 0, __VA_ARGS__);
#define pfatal(x) _pfatal(NULL, 0, x);
#endif

#endif
