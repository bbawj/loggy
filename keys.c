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
  case 'n': {
    Matches m = l->matches;
    if (m.len == 0) {
      break;
    }

    Match cur_match = m.matches[m.cur];
    int cur_row = l->cy + l->rowoff;
    if (cur_match.row >= cur_row) {
      l->rowoff = cur_match.row - cur_row;
    } else {
      l->cy = cur_row - cur_match.row;
    }

    if (cur_match.regmatch.rm_so >= l->c.cols) {
      int linelen = l->rows[cur_match.row].len;
      l->cx = l->c.cols;
      l->coloff = linelen - l->c.cols;
    } else {
      l->cx = cur_match.regmatch.rm_so;
    }
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
    if (regcomp(&search_regex, l->status_message.data, flags)) {
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
