#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    const size_t bytes = 64UL * 1024 * 1024;
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        fputs("cannot determine page size\n", stderr);
        return EXIT_FAILURE;
    }

    volatile unsigned char *memory = malloc(bytes);
    if (memory == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    for (size_t offset = 0; offset < bytes; offset += (size_t)page_size)
        memory[offset] = 1;

    printf("QNX_HEALTH_LOAD_READY pid=%ld bytes=%lu\n",
           (long)getpid(), (unsigned long)bytes);
    fflush(stdout);

    unsigned int remaining = 120;
    while (remaining != 0)
        remaining = sleep(remaining);

    free((void *)memory);
    return EXIT_SUCCESS;
}
