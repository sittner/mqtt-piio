#ifndef _SWITCH_H
#define _SWITCH_H

#include <confuse.h>
#include <gpiod.h>

typedef struct {
  const char *name;
  int gpio;

  const char *cmd_topic;
  const char *state_topic;

  const char *gpio_name;
  struct gpiod_line *gpio_line;

  int cmd;

  int output;
  int on_time;
  int on_timer;
} SWITCH_DATA_T;

int switch_init(cfg_t *cfg, SWITCH_DATA_T *sw);
void switch_cleanup(SWITCH_DATA_T *sw);

void switch_period(SWITCH_DATA_T *sw);

#endif

