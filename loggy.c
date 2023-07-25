#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "loggy.h"
#include "common.h"
#include "keys.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct termios original_termios;

void init(Loggy *l) {
  if (get_window_size(&l->c->rows, &l->c->cols) == -1) {
    die("get_window_size");
  }

  enable_raw_mode();

  l->rows = NULL;
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
  l->rows = realloc(l->rows, sizeof(Buffer) * (l->nrows + 1));

  int cur = l->nrows;

  l->rows[cur].len = len;
  l->rows[cur].data = malloc(len + 1);
  memcpy(l->rows[cur].data, s, len);
  l->rows[cur].data[len] = '\0';
  l->nrows++;
}

void refresh_screen(Loggy *l) {
  Buffer temp = {0, NULL};
  buf_append(&temp, "\x1b[?25l", 6);
  buf_append(&temp, "\x1b[H", 3);

  draw_screen(l, &temp);

  buf_append(&temp, "\x1b[H", 3);
  buf_append(&temp, "\x1b[?25l", 6);

  write(STDOUT_FILENO, temp.data, temp.len);
  free(temp.data);
}

void draw_screen(Loggy *l, Buffer *b) {
  Config *c = l->c;
  for (int i = 0; i < c->rows; i++) {

    if (i < l->nrows) {
      int len = l->rows[i].len;
      if (len > c->cols)
        len = c->cols;
      buf_append(b, l->rows[i].data, len);
    } else {
      buf_append(b, "~", 1);
    }

    buf_append(b, "\x1b[K", 3);
    if (i < c->rows - 1) {
      buf_append(b, "\r\n", 2);
    }
  }
}

void clear_screen(Buffer *b) {
  buf_append(b, "\x1b[2J", 4);
  buf_append(b, "\x1b[H", 3);
}

int main(int argc, char *argv[]) {
  Loggy l;
  init(&l);

  if (argc >= 2) {
    open_file(&l, argv[1]);
  }

  while (1) {
    refresh_screen(&l);
    process_key();
  }

  return 0;
}
