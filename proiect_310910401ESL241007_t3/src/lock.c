#include <fcntl.h>
#include <unistd.h>

void lock_region(int fd, off_t offset, size_t size) {
    struct flock fl = {0};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = size;
    fcntl(fd, F_SETLKW, &fl); // wait lock
}

void unlock_region(int fd, off_t offset, size_t size) {
    struct flock fl = {0};
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = size;
    fcntl(fd, F_SETLK, &fl);
}
