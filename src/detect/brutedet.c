#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "buffer.h"
#include "murmur.h"
#include "time.h"

#define MAX_LINELEN	4096		/* maximum line length in text mode */
#define CAPACITY 	100000
#define ERROR_RATE 	0.01

#define NR_BITMAPS	3		/* total number of bitmaps */

/* same constant as used in dablooms by Justin Wines at Bitly */
#define SALT_CONSTANT 	0x97c29b3a

/*
 * Perform the actual hashing for `key`
 *
 * Only call the hash once to get a pair of initial values (h1 and
 * h2). Use these values to generate all hashes in a quick loop.
 *
 * See paper by Kirsch, Mitzenmacher [2006]
 * http://www.eecs.harvard.edu/~michaelm/postscripts/rsa2008.pdf
 *
 * Slightly modified version from dablooms -- gvb
 */
inline static void
hash_func(const char * data, size_t datalen, uint32_t * hashes,
	int nfuncs, int counts_per_func)
{
	int i;
	uint32_t checksum[4], h1, h2;

	MurmurHash3_x64_128(data, datalen, SALT_CONSTANT, checksum);
	h1 = checksum[0];
	h2 = checksum[1];
	for (i = 0; i < nfuncs; i++) {
		hashes[i] = (h1 + i * h2) % counts_per_func;
	}
}

inline static char *
string_replace(const char * input, const char * key, const char * replace)
{
	char * p, * res;
	size_t off, maxsz;

	maxsz = 8192;
	off = 0;
	res = xmalloc(maxsz);

	p = strstr(input, key);
	while (p) {
		if (off + p-input+strlen(replace) > maxsz)
			fatal("string too long");
		memcpy(res + off, input, p-input);
		off = off + p-input;
		memcpy(res + off, replace, strlen(replace));
		off = off + strlen(replace);
		p+=strlen(key);
		input = p;
		p = strstr(input, key);
	}
	if (off + strlen(input) > maxsz)
		fatal("string too long");
	memcpy(res + off, input, strlen(input));
	return res;
}

static void
exec_cmd(const char * base_cmd, const char * key)
{
	char * cmd;
	int ret;

	cmd = string_replace(base_cmd, "KEY", key);
	ret = system(cmd);
	if (ret  == -1) pfatal("system");	
}

static void
usage(const char * arg0)
{
	fprintf(stderr, "%s [options] <t1> <t2> <t3> \"echo added KEY\"\n\n", arg0);
	fprintf(stderr, "Parameters t1, t2, t3 are the integer tresholds for the ");
	fprintf(stderr, "10 second-,\n1 minute-, 10 minute-bucket respectively.\n\n");
	fprintf(stderr, "The last argument is the command to execute once a treshold\n");
	fprintf(stderr, "is passed. The word KEY will be replaced with the key in the\n");
	fprintf(stderr, "bloomfilter. This will most likely be an IP address.\n\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, " -c <capacity>         bloom filter capacity ");
	fprintf(stderr, "(default: %u)\n", CAPACITY);
	fprintf(stderr, " -e <error rate>       allowed error rate");
	fprintf(stderr, "(default: %.3f)\n", ERROR_RATE);
	fprintf(stderr, " -h                    help (this screen)\n\n");
	fprintf(stderr, "The following example echo's a warning to a logfile\n");
	fprintf(stderr, "and it will accept a maximum of 5 requests every 10 seconds,\n");
	fprintf(stderr, "a maximum of 20 requests every minute and a maximum of 40\n");
	fprintf(stderr, "requests every 10 minutes.\n\n");
	fprintf(stderr, " %s 5 20 40 \"echo treshold reached for KEY >> /tmp/logfile.txt\"\n", arg0);
	exit(EXIT_FAILURE);
}

int
main(int argc, char ** argv)
{
	struct termios tio;
	struct buffer * data;
	struct pollfd pfd;
	size_t off;
	char line[MAX_LINELEN], *p, * cmd;
	int ret, fd, c;
	uint32_t capacity, nfuncs, counts_per_func, size, i, j, index, min;
	uint32_t * bitmaps[NR_BITMAPS], * hashes;
	uint32_t bitmap_diffs[NR_BITMAPS] = {10, 60, 600};
	uint32_t bitmap_max[NR_BITMAPS] = {2, 10, 50};
	struct taia timestamps[NR_BITMAPS], now, diff;
	double error_rate;


	while ((c = getopt(argc, argv, "c:e:h")) != -1) {
		switch (c) {
		case 'h':
			usage(argc > 0 ? argv[0] : "(unknown)");
			break;
		case 'c':
			capacity = atoi(optarg);
			break;
		case 'e':
			error_rate = atof(optarg);
			break;
		}
	}

	if (argc-optind != NR_BITMAPS+1) {
		fprintf(stderr, "not enough (or too many) arguments supplied\n\n");
		usage(argc > 0 ? argv[0] : "(unknown)");
	}

	/* get the tresholds from the command line */
	for (i=0;i<NR_BITMAPS;i++) {
		bitmap_diffs[i] = atoi(argv[optind+i]);
		printf("%i\n", bitmap_diffs[i]);
	}

	/* get the command which will be executed */
	cmd = argv[optind+3];

	/* calculate the bloom filter parameters */
	capacity = CAPACITY;
	error_rate = ERROR_RATE;
	nfuncs = (int)ceil(log(1/error_rate) / log(2));
	counts_per_func = (int) ceil(capacity * fabs(log(error_rate))
		/(nfuncs * pow(log(2), 2)));
	size = nfuncs * counts_per_func * sizeof(uint32_t);

	debug("nfuncs: %u, counts_per_func: %u, size: %u, bits: %lu\n",
		nfuncs, counts_per_func, size, size/sizeof(uint32_t));

	/* allocate all the bitmaps necessary and set timestamps */
	taia_now(&now);
	for (i=0;i<NR_BITMAPS;i++) {
		bitmaps[i] = xmalloc(size);
		memcpy(&(timestamps[i]), &now, sizeof(struct taia));
	}

	/* allocate hash structure used to calculate bitmap indices */
	hashes = xmalloc(nfuncs * sizeof(uint32_t));

	/* turn of line buffering */
	tcgetattr(STDIN_FILENO, &tio);
	tio=tio;
	tio.c_lflag &=(~ICANON);
	tcsetattr(STDIN_FILENO,TCSANOW, &tio);

	/* prepare poll */
	fd = STDIN_FILENO;
	pfd.fd = fd;
	pfd.events = POLLIN;
	data = buffer_new();

	/* event loop */
	while (1) {
		/* timeout every second */
		ret = poll(&pfd, 1, 1000);
		if (ret < 0) pfatal("poll");

		if (pfd.revents == POLLIN) {
			buffer_fd_append(data, fd, fd_ravail(fd));

			ret = buffer_findchar(data, '\n', &off);	
		nextline:	
			if (ret < 0) {
				if (buffer_avail(data)>=MAX_LINELEN) {
					fatal("line too long");
				}
				continue;
			}
			else if (off >= MAX_LINELEN-1) {
				fatal("line too long");
			}
			buffer_consume(data, line, off+1);	
			buffer_reset(data);
			line[off] = 0;
		
			/* strip whitespace between id and data and
			 * rejoin the datasuch that it's only
			 * separated by a space */
			p = line;
			while (*p) {
				if (*p == ' ' || *p == '\t') {
					*p = 0;
					p++;
					break;
				}
				p++;
			}
			while (*p == ' ' || *p == '\t') p++;
			if (!strlen(line) || p == line+off)
				fatal("invalid line");

			/* reset the data so that the id and the data
				are separated by just only one space */
			memmove(line+strlen(line)+1, p, strlen(p));
			*(line+strlen(line)) = 0;
			*(line+strlen(line)+1+strlen(p))=0;

			/* calculate the hashes */
			hash_func(line, strlen(line), hashes,
				nfuncs, counts_per_func);

			/* loop over the bitmaps and see if for any of the
			 * maps the maximum limits were reached */
			for (j=0;j<NR_BITMAPS;j++) {
				min = UINT32_MAX;
				for (i=0;i<nfuncs;i++) {
					off = i * counts_per_func;
					index = hashes[i] + off;	
					bitmaps[j][index] += 1;	
					if (bitmaps[j][index] < min) {
						min = bitmaps[j][index];
					}
				}
				if (min > bitmap_max[j]) {
					debug("treshold reached for %s\n",
						line);
					exec_cmd(cmd, line);
				}
			}
	
			ret = buffer_findchar(data, '\n', &off);	
			if (ret >= 0) goto nextline;
		}

		/* reset the bitmaps if they've passed the time treshold */
		taia_now(&now);
		for (i=0;i<NR_BITMAPS;i++) {
			taia_diff(&timestamps[i], &now, &diff);
			if (diff.sec.x >= bitmap_diffs[i]) {
				memcpy(&(timestamps[i]), &now,
					sizeof(struct taia));
				memset(bitmaps[i], 0, size);
			}
		}	
	}
}
