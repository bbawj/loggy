#include <stddef.h>
#include <termios.h>

typedef struct {
  int rows;
  int cols;
} Config;

typedef struct {
  int len;
  char *data;
} Buffer;

typedef struct {
  Config *c;
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
void refresh_screen(Loggy *l);
void clear_screen(Buffer *buf);
