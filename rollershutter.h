#ifndef _ROLLERSHUTTER_H_
#define _ROLLERSHUTTER_H_

typedef struct {
  const char *name;
  int gpio_up;
  int gpio_down;
  int time_full;
  int time_extra;
  int time_pause;

  int position;
  int output;
  int up_timer;
  int down_timer;
} ROLLSH_DATA_T;

#endif

