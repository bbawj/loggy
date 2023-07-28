#ifndef LOGGY_H_
#define LOGGY_H_

#include <stddef.h>
#include <termios.h>

typedef enum { NORMAL, SEARCH } mode;

typedef struct {
  int rows;
  int cols;
} Config;

typedef struct {
  int len;
  char *data;
} Buffer;

typedef struct Loggy {
  Config c;
  mode mode;
  char *filename;
  Buffer status_message;
  int cx, cy;
  int rowoff, coloff;
  Buffer *rows;
  int nrows;
  int ncols;
} Loggy;

void enable_raw_mode();
void disable_raw_mode();

int get_window_size(int *rows, int *cols);

void buf_append(Buffer *buf, const char *s, int len);

void open_file(Loggy *l, char *filename);

void row_append(Loggy *l, char *s, size_t len);
void draw_screen(Loggy *l, Buffer *buf);
void draw_status_bar(Loggy *l, Buffer *b);
void draw_status_message(Loggy *l, Buffer *b, char *status_message);
void clear_status_message(Loggy *l);
void refresh_screen(Loggy *l);

#endif // LOGGY_H_
