#ifndef CTOOLS_CIRCULAR_BUFFER_IN_LINUX
#define CTOOLS_CIRCULAR_BUFFER_IN_LINUX

/**
 * A circular buffer.
 *
 * Internally, the buffer employs doubled address space,
 * meaning that `buffer[i] == buffer[i + buffer_size]` is always true.
 * This completely avoids the use of modulo statements, improving performance for programs
 * using large amounts of smaller read and write operations on this circular buffer.
 *
 * The downside of this is that the internal buffer's size must be aligned with the system's memory
 * page size, making this implementation less suitable for situations requiring tight memory
 * allocations.
 */
struct ct_cbuf {
    void *buffer;
    int memfd;
    unsigned int capacity;
    unsigned int head;
    unsigned int tail;
};

int ct_cbuf_init(struct ct_cbuf *cbuf, const unsigned int min_capacity);

void ct_cbuf_exit(struct ct_cbuf *cbuf);

/**
 * @brief Reads data from a circular buffer.
 *
 * @param[in] cbuf Pointer to the circular buffer structure.
 * @param[out] dst Pointer to the destination buffer where data will be read into.
 * @param[in] read_count The number of bytes to read from the circular buffer.
 *
 * @return The number of bytes actually read from the circular buffer,
 *         or a negative value on error.
 *
 * @note The caller is responsible for ensuring that @p dst points to
 *       a valid buffer with sufficient space to hold @p read_count bytes.
 */
int ct_cbuf_read(struct ct_cbuf *cbuf, void *dst, unsigned int read_count);

/**
 * @brief Writes data to a circular buffer
 * @param cbuf Pointer to the circular buffer structure
 * @param src Pointer to the source data to write
 * @param write_count Number of bytes to write
 * @return Number of bytes successfully written, or negative value on error
 */
int ct_cbuf_write(struct ct_cbuf *cbuf, const void *src, const unsigned int write_count);

unsigned int ct_cbuf_space_left(const struct ct_cbuf *cbuf);

unsigned int ct_cbuf_space_occupied(const struct ct_cbuf *cbuf);

#endif // CTOOLS_CIRCULAR_BUFFER_IN_LINUX
