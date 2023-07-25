#include "keys.h"
#include "common.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

char read_key() {
  int nread;
  char buf;

  while ((nread = read(STDIN_FILENO, &buf, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  return buf;
}

void process_key() {
  char c = read_key();

  switch (c) {
  case CTRL_KEY('q'):
    exit(0);
    break;
  default:
    break;
  }
}
