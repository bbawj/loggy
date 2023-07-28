#include "keys.h"
#include "common.h"
#include "loggy.h"
#include <errno.h>
#include <stdio.h>
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

void process_key_normal(Loggy *l) {
  char c = read_key();

  switch (c) {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  case 'h':
  case 'j':
  case 'k':
  case 'l':
    move_cursor(l, c);
    break;
  case 'G': {
    int times = l->nrows - l->cy;
    while (times > 0) {
      move_cursor(l, 'j');
      times--;
    }
  } break;
  case '0':
    l->coloff = 0;
    l->cx = 0;
    break;
  case '$': {
    int linelen = l->rows[l->rowoff + l->cy].len;
    l->coloff = linelen - l->c.cols;
    if (l->coloff < 0)
      l->coloff = 0;
    l->cx = linelen - l->coloff - 1;
    if (l->cx < 0)
      l->cx = 0;
  } break;
  case '/':
    l->mode = SEARCH;
    l->status_message.data[0] = '/';
    l->status_message.len++;
    break;
  default:
    break;
  }
}

void move_cursor(Loggy *l, char key) {
  switch (key) {
  case 'h':
    if (l->cx > 0) {
      l->cx--;
    } else if (l->coloff > 0) {
      l->coloff--;
    }
    break;
  case 'j':
    if (l->cy < l->c.rows - 1) {
      l->cy++;
    } else if (l->rowoff + l->cy < l->nrows - 1) {
      l->rowoff++;
    }
    break;
  case 'k':
    if (l->cy > 0) {
      l->cy--;
    } else if (l->rowoff > 0) {
      l->rowoff--;
    }
    break;
  case 'l':
    if (l->cx < l->c.cols - 1) {
      l->cx++;
    } else if (l->coloff + l->cx < l->ncols - 1) {
      l->coloff++;
    }
    break;
  }
}

void process_key_search(Loggy *l) {
  char c = read_key();

  switch (c) {
  case 0x1b:
    l->mode = NORMAL;
    clear_status_message(l);
    break;
  default:
    if (l->status_message.len + 1 > l->c.cols) {
      break;
    }
    l->status_message.data[l->status_message.len++] = c;
    break;
  }
}
