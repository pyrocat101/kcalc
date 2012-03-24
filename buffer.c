#include <stdarg.h>
#include <linux/kernel.h>
#include "buffer.h"

/* the expression buffer */
inbuf expr_buf = {0};

/* the output ring buffer */
ring result_ring = {0};

int inbuf_getchar(inbuf *buf) {
    if (buf->rpos >= buf->count)
        return EOF;
    else
        return buf->buf[buf->rpos++];
}

int ring_sprintf32(ring *buf, const char *fmt, ...) {
    char tmp_buf[32];
    va_list args;
    int i, j, copy_start;

    va_start(args, fmt);
    i = vsnprintf(tmp_buf, 32, fmt, args);
    va_end(args);

    /* copy chars in temp buffer into the ring */
    copy_start = (buf->head + buf->count) % RING_SIZE;
    for (j = 0; j < i; j++) {
        buf->buf[copy_start] = tmp_buf[j];
        if (RING_SIZE == buf->count)
            buf->head = (buf->head + 1) % RING_SIZE;
        else
            ++buf->count;
        copy_start = (copy_start + 1) % RING_SIZE;
    }

    return i;
}
