#include "keys.h"
#include "common.h"
#include "loggy.h"
#include <errno.h>
#include <regex.h>
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
  case 'n':
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
    }
    break;
  case 'j':
    if (l->cy + 1 < l->nrows) {
      l->cy++;
    }
    break;
  case 'k':
    if (l->cy > 0) {
      l->cy--;
    }
    break;
  case 'l':
    if (l->cx + 1 < l->ncols) {
      l->cx++;
    }
    break;
  case 'n': {
    Matches m = l->matches;
    if (m.len == 0) {
      break;
    }

    Match cur_match = m.matches[m.cur];
    l->cy = cur_match.row;
    l->cx = cur_match.regmatch.rm_so;

    l->matches.cur =
        l->matches.cur + 1 == l->matches.len ? 0 : ++l->matches.cur;
  } break;
  }
}

void process_key_search(Loggy *l) {
  char c = read_key();

  switch (c) {
  case 0x1b:
    l->mode = NORMAL;
    clear_status_message(l);
    break;
  case 0xd: {
    l->mode = NORMAL;
    int flags = 0;
    regex_t search_regex;
    if (regcomp(&search_regex, &l->status_message.data[1], flags)) {
      die("regcomp");
    }
    find(l, search_regex);
    clear_status_message(l);
  } break;
  default:
    if (l->status_message.len + 1 > l->c.cols) {
      break;
    }
    l->status_message.data[l->status_message.len++] = c;
    break;
  }
}
