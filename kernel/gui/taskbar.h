#ifndef TASKBAR_H
#define TASKBAR_H

#include <stdint.h>

void taskbar_init(void);
void taskbar_draw(void);
void taskbar_on_click(int x, int y);

// Estado del Start Menu
extern int taskbar_start_open;

#endif
