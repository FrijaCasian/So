#ifndef LOCK_H
#define LOCK_H

#include <unistd.h>

void lock_region(int fd, off_t offset, size_t size);
void unlock_region(int fd, off_t offset, size_t size);

#endif
