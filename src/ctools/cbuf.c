#define _GNU_SOURCE
#include <linux/memfd.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ctools/cbuf.h"

static inline size_t page_align_up(size_t size, size_t page_size) {
    return (size + page_size - 1) & ~(page_size - 1);
}

int ct_cbuf_init(struct ct_cbuf *buf, const unsigned int min_capacity) {
    // The capacity must be page-aligned
    unsigned int capacity = page_align_up(min_capacity, sysconf(_SC_PAGESIZE));

    // Create anonymous memory object
    int fd = memfd_create("mirror", 0);
    if (fd < 0) {
        perror("memfd_create");
        return -1;
    }

    if (ftruncate(fd, capacity)) {
        perror("ftruncate");
        return -1;
    }

    // Reserve address space
    void *addr = mmap(NULL, capacity * 2, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == (void *)-1) {
        perror("mmap");
        return -1;
    }

    // Map both halves of the address space
    mmap(addr, capacity, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    mmap(addr + capacity, capacity, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

    *buf = (struct ct_cbuf){
        .buffer = addr,
        .memfd = fd,
        .capacity = capacity,
        .head = 0,
        .tail = 0,
    };

    return 0;
}

void ct_cbuf_exit(struct ct_cbuf *buf) {
    munmap(buf->buffer, buf->capacity * 2);
    close(buf->memfd);

    // Zero out the struct
    memset(buf, 0, sizeof(struct ct_cbuf));
}

int ct_cbuf_read(struct ct_cbuf *buf, void *dst, unsigned int read_count) {
    // Reduce the read count if there is not enough data available
    if (read_count > ct_cbuf_space_occupied(buf))
        read_count = ct_cbuf_space_occupied(buf);

    // Perform the read operation
    memcpy(dst, buf->buffer + buf->tail, read_count);

    // Update the tail
    buf->tail += read_count;

    // When the tail enters the second half of the address space,
    // move both indices back into the first half of the address space.
    if (buf->tail >= buf->capacity) {
        buf->head -= buf->capacity;
        buf->tail -= buf->capacity;
    }

    // Return the amount of bytes read
    return read_count;
}

int ct_cbuf_write(struct ct_cbuf *buf, const void *src, const unsigned int write_count) {
    // Disallow any call to write more bytes than
    // how much space is currently available in the buffer.
    if (write_count > ct_cbuf_space_left(buf))
        return -1;

    // Perform the write operation
    memcpy(buf->buffer + buf->head, src, write_count);

    // Update the head
    buf->head += write_count;

    // Notify success
    return write_count;
}

unsigned int ct_cbuf_space_left(const struct ct_cbuf *buf) {
    return buf->capacity - ct_cbuf_space_occupied(buf);
}

unsigned int ct_cbuf_space_occupied(const struct ct_cbuf *buf) { return buf->head - buf->tail; }
