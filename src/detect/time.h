#ifndef TIME_H
  #define TIME_H

/* The tai and taia routines are based on djb's libtai which is placed in the
 * public domain. See http://cr.yp.to/libtai.html for details. The taia code
 * was slightly modified to only take into account nano-second based timing.
 * All the atto-second related code was removed to save some space when packing
 * taia structures on disk. */

struct tai {
	uint64_t x;
};

void tai_now(struct tai *);
#define TAI_PACK 8
void tai_pack(char *, const struct tai *);
void tai_unpack(const char *, struct tai *);

#define tai_unix(t,u) ((void)((t)->x = 4611686018427387914ULL + (uint64_t)(u)))

struct taia {
	struct tai sec;
	unsigned long nano; /* 0...999999999 */
};

void taia_now(struct taia *);

#define TAIA_PACK 12
void taia_pack(char *, const struct taia *);
void taia_unpack(const char *, struct taia *);

int taia_less(struct taia *, struct taia *);
int taia_leq(struct taia *, struct taia *);

void taia_diff(struct taia *, struct taia *, struct taia *);

int timestr_parse(const char *, struct taia *, struct taia *);

#endif
