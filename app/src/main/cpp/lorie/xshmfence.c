#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

struct xshmfence {
    uint8_t is_futex;
    void* real;
};

// This code is needed to use both futex and pthread implementations depending on size of shared memory fragment we get from X client.

int xshmfence_futex_trigger(struct xshmfence *f);
int xshmfence_futex_await(struct xshmfence *f);
int xshmfence_futex_query(struct xshmfence *f);
void xshmfence_futex_reset(struct xshmfence *f);
struct xshmfence * xshmfence_futex_map_shm(int fd);
void xshmfence_futex_unmap_shm(struct xshmfence *f);
int xshmfence_futex_alloc_shm(void);

int xshmfence_pthread_trigger(struct xshmfence *f);
int xshmfence_pthread_await(struct xshmfence *f);
int xshmfence_pthread_query(struct xshmfence *f);
void xshmfence_pthread_reset(struct xshmfence *f);
struct xshmfence * xshmfence_pthread_map_shm(int fd);
void xshmfence_pthread_unmap_shm(struct xshmfence *f);

int xshmfence_trigger(struct xshmfence *f) {
    return f->is_futex ? xshmfence_futex_trigger(f->real) : xshmfence_pthread_trigger(f->real);
}

int xshmfence_await(struct xshmfence *f) {
    return f->is_futex ? xshmfence_futex_await(f->real) : xshmfence_pthread_await(f->real);
}

int xshmfence_query(struct xshmfence *f) {
    return f->is_futex ? xshmfence_futex_query(f->real) : xshmfence_pthread_query(f->real);
}

void xshmfence_reset(struct xshmfence *f) {
    if (f->is_futex) xshmfence_futex_reset(f->real); else xshmfence_pthread_reset(f->real);
}

int xshmfence_alloc_shm(void) {
    int fd = -1;
#ifdef __NR_memfd_create
    fd = syscall(__NR_memfd_create, "xshmfence", MFD_CLOEXEC | MFD_ALLOW_SEALING);
#endif

    if (fd < 0) {
        return -1;
    }

    if (ftruncate(fd, sizeof(uint32_t)) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

struct xshmfence * xshmfence_map_shm(int fd) {
    struct xshmfence* f = calloc(1, sizeof(*f));
    struct stat st = {0};

    if (!f)
        return NULL;

    fstat(fd, &st);
    f->is_futex = st.st_size == sizeof(uint32_t);
    f->real = f->is_futex ? xshmfence_futex_map_shm(fd) : xshmfence_pthread_map_shm(fd);
    if (!f->real) {
        free(f);
        f = NULL;
    }

    return f;
}

void xshmfence_unmap_shm(struct xshmfence *f) {
    if (f->is_futex) xshmfence_futex_unmap_shm(f->real); else xshmfence_pthread_unmap_shm(f->real);
    free(f);
}
