#ifndef _KCALC_BUFFER_H_
#define _KCALC_BUFFER_H_

/* EOF */
#define EOF -1

/* the size of input buffer */
#define INBUF_SIZE  4096

/* the size of ring */
#define RING_SIZE   1024

typedef struct _inbuf {
    int count;
    int rpos;
    char buf[INBUF_SIZE];
} inbuf;

typedef struct _ring {
    int count;
    int head;
    char buf[RING_SIZE];
} ring;

extern inbuf expr_buf;
extern ring result_ring;

/*
 * inbuf_getchar: get a character from input buffer
 * @buf: the input buffer
 * Note: if read to end, this function will return EOF.
 */

int inbuf_getchar(inbuf *buf);

/*
 * ring_sprintf32: format a string no longer than 32 and place it in a buffer
 * @buf: the ring buffer to place the result into
 * @fmt: the format string to use @... arguments for the format string
 * @...: variable arguments
 * Note: the result string will be truncated at 32
 */

int ring_sprintf32(ring *buf, const char *fmt, ...);

#endif
