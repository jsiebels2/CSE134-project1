#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096

enum { COPY_OK = 0, COPY_READ_ERR = 1, COPY_WRITE_ERR = 2 };

// Write all n bytes of buf to fd, looping on partial writes.
// Returns 0 on success, -1 on error (errno is set).
static int write_all(int fd, const char *buf, ssize_t n) {
  ssize_t total = 0;
  while (total < n) {
    ssize_t w = write(fd, buf + total, n - total);
    if (w == -1) {
      return -1;
    }
    total += w;
  }
  return 0;
}

// Copy everything from fd to stdout. name is used in error messages.
static int copy_fd(int fd, const char *name) {
  char buf[BUF_SIZE];
  ssize_t n;

  while ((n = read(fd, buf, sizeof buf)) > 0) {
    if (write_all(STDOUT_FILENO, buf, n) == -1) {
      warn("write error");
      return COPY_WRITE_ERR;
    }
  }

  if (n == -1) {
    warn("%s", name);
    return COPY_READ_ERR;
  }

  return COPY_OK;
}

int main(int argc, char *argv[]) {
  int status = 0;

  if (argc == 1) {
    return copy_fd(STDIN_FILENO, "-") == COPY_OK ? 0 : 1;
  }

  for (int i = 1; i < argc; i++) {
    int rc;

    if (strcmp(argv[i], "-") == 0) {
      rc = copy_fd(STDIN_FILENO, "-");
    } else {
      int fd = open(argv[i], O_RDONLY);
      if (fd == -1) {
        warn("%s", argv[i]);
        status = 1;
        continue;
      }
      rc = copy_fd(fd, argv[i]);
      close(fd);
    }

    if (rc != COPY_OK) {
      status = 1;
      if (rc == COPY_WRITE_ERR) {
        return status;
      }
    }
  }

  return status;
}
