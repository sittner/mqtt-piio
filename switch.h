#ifndef _SWITCH_H
#define _SWITCH_H

typedef struct {
  const char *name;
  int gpio;

  int output;
  int on_time;
  int on_timer;
} SWITCH_DATA_T;

#endif

