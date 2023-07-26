#include "loggy.h"

#define CTRL_KEY(k) ((k)&0x1f)

char read_key();
void process_key(Loggy *l);
void move_cursor(Loggy *l, char key);
