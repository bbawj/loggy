#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "loggy.h"
#include "common.h"
#include "keys.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct termios original_termios;

void init(Loggy *l) {
  if (get_window_size(&l->c.rows, &l->c.cols) == -1) {
    die("get_window_size");
  }
  l->c.rows -= 2;

  enable_raw_mode();

  l->matches = (Matches){.matches = NULL, .len = 0, .cur = 0};

  char status_buffer[l->c.cols];
  l->status_message = (Buffer){.len = 0, .data = malloc(sizeof(status_buffer))};
  l->mode = NORMAL;

  l->filename = NULL;
  l->rows = NULL;
  l->cx = 0;
  l->cy = 0;
  l->rowoff = 0;
  l->coloff = 0;
  l->ncols = 0;
  l->nrows = 0;
}

void enable_raw_mode() {
  if (tcgetattr(STDIN_FILENO, &original_termios) == -1)
    die("tcgetattr");
  atexit(disable_raw_mode);

  struct termios raw = original_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

void disable_raw_mode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1)
    die("tcsetattr");
}

void buf_append(Buffer *buf, const char *s, int len) {
  if (len == 0)
    return;
  char *new = realloc(buf->data, buf->len + len);
  if (new == NULL) {
    return;
  }

  memcpy(&new[buf->len], s, len);
  buf->data = new;
  buf->len += len;
}

int get_window_size(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

void open_file(Loggy *l, char *filename) {
  free(l->filename);
  l->filename = strdup(filename);

  FILE *fp = fopen(filename, "r");
  if (!fp) {
    die("fopen");
  }

  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;
  while ((length = getline(&line, &capacity, fp)) != -1) {
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
      length--;

    row_append(l, line, length);
  }
  free(line);
  fclose(fp);
}

void row_append(Loggy *l, char *s, size_t len) {
  if (len > l->ncols) {
    l->ncols = len;
  }

  l->rows = realloc(l->rows, sizeof(Buffer) * (l->nrows + 1));

  int cur = l->nrows;

  l->rows[cur].len = len;
  l->rows[cur].data = malloc(len + 1);
  memcpy(l->rows[cur].data, s, len);
  l->rows[cur].data[len] = '\0';
  l->nrows++;
}

// /row
void refresh_screen(Loggy *l) {
  Buffer temp = {0, NULL};
  buf_append(&temp, "\x1b[?25l", 6);
  buf_append(&temp, "\x1b[H", 3);

  draw_screen(l, &temp);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", l->cy + 1, l->cx + 1);
  buf_append(&temp, buf, strlen(buf));

  buf_append(&temp, "\x1b[?25h", 6);

  write(STDOUT_FILENO, temp.data, temp.len);
  free(temp.data);
}

void find(Loggy *l, regex_t reg) {
  for (int i = 0; i < l->nrows; i++) {
    regmatch_t pmatch[1];

    char *cur_line = l->rows[i].data;
    regoff_t off, length;
    if (regexec(&reg, cur_line, sizeof(pmatch) / sizeof(pmatch[0]), pmatch,
                0)) {
      continue;
    }
    l->matches.matches =
        realloc(l->matches.matches, sizeof(Match) * (l->matches.len + 1));
    l->matches.matches[l->matches.len] =
        (Match){.regmatch = pmatch[0], .row = i};
    l->matches.len++;
  }
}

void draw_screen(Loggy *l, Buffer *b) {
  Config c = l->c;

  for (int i = 0; i < c.rows; i++) {
    buf_append(b, "\x1b[K", 3);

    int colstart = l->coloff;
    int offset = l->rowoff;

    if (offset + i < l->nrows) {
      int len = l->rows[offset + i].len - colstart;

      if (len > c.cols)
        len = c.cols;

      if (len < 0) {
        colstart = 0;
        len = 0;
      }

      assert(colstart <= l->rows[offset + i].len);

      buf_append(b, &l->rows[offset + i].data[colstart], len);
    } else {
      buf_append(b, "~", 1);
    }

    buf_append(b, "\r\n", 2);
  }

  draw_status_bar(l, b);
  draw_status_message(l, b, "");
}

void draw_status_bar(Loggy *l, Buffer *b) {
  buf_append(b, "\x1b[7m", 4);

  char left_status[80];
  int len = snprintf(left_status, sizeof(left_status), "%.20s",
                     l->filename ? l->filename : "[No Name]");
  if (len > l->c.cols)
    len = l->c.cols;
  char right_status[l->c.cols - len];
  int rlen = snprintf(right_status, sizeof(right_status), "%d lines", l->nrows);
  buf_append(b, left_status, len);
  while (len < l->c.cols) {
    if (len + rlen == l->c.cols) {
      buf_append(b, right_status, rlen);
      break;
    } else {
      buf_append(b, " ", 1);
      len++;
    }
  }

  buf_append(b, "\x1b[m", 3);
}

void draw_status_message(Loggy *l, Buffer *b, char *status_message) {
  buf_append(b, l->status_message.data, l->status_message.len);
  int padding = l->c.cols - l->status_message.len;
  while (padding > 0) {
    buf_append(b, " ", 1);
    padding--;
  }
}

void clear_status_message(Loggy *l) {
  free(l->status_message.data);
  char status_buffer[l->c.cols];
  l->status_message = (Buffer){.len = 0, .data = malloc(sizeof(status_buffer))};
}

int main(int argc, char *argv[]) {
  Loggy l;
  init(&l);

  if (argc >= 2) {
    open_file(&l, argv[1]);
  }

  while (1) {
    refresh_screen(&l);
    switch (l.mode) {
    case NORMAL:
      process_key_normal(&l);
      break;
    case SEARCH:
      process_key_search(&l);
      break;
    default:
      break;
    }
  }

  return 0;
}
