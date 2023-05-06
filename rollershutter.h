#ifndef _ROLLERSHUTTER_H_
#define _ROLLERSHUTTER_H_

#include <confuse.h>
#include <gpiod.h>

typedef struct {
  const char *name;
  int pin_up;
  int pin_down;
  int time_full;
  int time_extra;
  int time_pause;

  const char *cmd_topic;
  const char *state_topic;

  const char *pin_up_name;
  struct gpiod_line *pin_up_line;
  const char *pin_down_name;
  struct gpiod_line *pin_down_line;

  int cmd;

  int position;
  int output;
  int up_timer;
  int down_timer;
} ROLLSH_DATA_T;

int rollsh_init(cfg_t *cfg, ROLLSH_DATA_T *rollsh);
void rollsh_cleanup(ROLLSH_DATA_T *rollsh);

void rollsh_period(ROLLSH_DATA_T *rollsh);

#endif

