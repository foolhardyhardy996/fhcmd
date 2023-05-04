

#define FHBUF_CAP (128*1024)
struct fhbuf {
    uint8_t base[BUF_CAP];
    int head;
    int len;
};

void fhbuf_init(struct fhbuf *buf) {
    buf->head = 0;
    buf->len = 0;
}

void fhbuf_compact(struct fhbuf *buf) {
    memmove(buf->base, buf->base + buf->head, buf->len);
    buf->head = 0;
}