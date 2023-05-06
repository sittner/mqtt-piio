#include "switch.h"
#include "mqtt.h"
#include "gpio.h"
#include "timer.h"

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int switch_init(cfg_t *cfg, SWITCH_DATA_T *sw) {
  sw->name = strdup(cfg_title(cfg));
  sw->pin = cfg_getint(cfg, "pin");
  sw->on_time = cfg_getint(cfg, "on_time");

  asprintf((char **) &sw->cmd_topic, "%s/%s/cmd", mqtt_conf.base_topic, sw->name);
  asprintf((char **) &sw->state_topic, "%s/%s/state", mqtt_conf.base_topic, sw->name);

  asprintf((char **) &sw->pin_name, "piio.%s", sw->name);
  sw->pin_line = gpio_request_output(sw->pin, sw->pin_name);
  if (sw->pin_line == NULL) {
    return -1;
  }

  sw->do_restore = 1;

  return 0;
}

void switch_cleanup(SWITCH_DATA_T *sw) {
  free((void *) sw->name);
  free((void *) sw->cmd_topic);
  free((void *) sw->state_topic);
  free((void *) sw->pin_name);

  if (sw->pin_line != NULL) {
    gpiod_line_release(sw->pin_line);
  }
}

void switch_cmd(SWITCH_DATA_T *sw, const char *cmd) {
  sw->do_restore = 0;
  if (strcmp("ON", cmd) == 0) {
    sw->cmd = SWITCH_CMD_ON;
  }
  if (strcmp("OFF", cmd) == 0) {
    sw->cmd = SWITCH_CMD_OFF;
  }
}

void switch_period(SWITCH_DATA_T *sw) {
  int last_output;

  last_output = sw->output;
  if (sw->cmd == SWITCH_CMD_ON) {
    sw->output = 1;
  }
  if (sw->cmd == SWITCH_CMD_OFF) {
    sw->output = 0;
  }
  sw->cmd = SWITCH_CMD_NOP;

  if (sw->output && sw->on_timer > 0) {
    sw->on_timer -= TIMER_PERIOD_MS;
    if (sw->on_timer <= 0) {
      sw->output = 0;
    }
  }
  if (!sw->output) {
    sw->on_timer = sw->on_time;
  }

  if (sw->output != last_output) {
    mqtt_publish_switch(sw->state_topic, sw->output);
  }

  gpiod_line_set_value(sw->pin_line, sw->output);
}

