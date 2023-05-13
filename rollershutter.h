#ifndef _ROLLERSHUTTER_H_
#define _ROLLERSHUTTER_H_

#include <confuse.h>
#include <gpiod.h>
#include <mosquitto.h>

#define ROLLSH_CMD_NOP    0
#define ROLLSH_CMD_STOP   1
#define ROLLSH_CMD_OPEN   2
#define ROLLSH_CMD_CLOSE  3

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
  int do_restore;
  int last_pos;

  int position;
  int output;
  int up_timer;
  int down_timer;

} ROLLSH_DATA_T;

void rollsh_init(void);
int rollsh_configure(cfg_t *cfg);
void rollsh_unconfigure(void);

int rollsh_startup(void);
void rollsh_shutdown(void);

void rollsh_mqtt_subscribe(struct mosquitto *mosq);
void rollsh_mqtt_msg(const char *topic, const char *msg);

void rollsh_period(void);

#endif

