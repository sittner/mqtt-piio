#include "rollershutter.h"
#include "mqtt.h"
#include "gpio.h"

#include <stdlib.h>
#include <string.h>

int rollsh_init(cfg_t *cfg, ROLLSH_DATA_T *rollsh) {
  rollsh->name = strdup(cfg_title(cfg));
  rollsh->gpio_up = cfg_getint(cfg, "gpio_up");
  rollsh->gpio_down = cfg_getint(cfg, "gpio_down");
  rollsh->time_full = cfg_getint(cfg, "time_full");
  rollsh->time_extra = cfg_getint(cfg, "time_extra");
  rollsh->time_pause = cfg_getint(cfg, "time_pause");

  asprintf((char **) &rollsh->cmd_topic, "%s/%s/cmd", mqtt_conf.base_topic, rollsh->name);
  asprintf((char **) &rollsh->state_topic, "%s/%s/state", mqtt_conf.base_topic, rollsh->name);

  asprintf((char **) &rollsh->gpio_up_name, "piio.rsh.%s.up", rollsh->name);
  rollsh->gpio_up_line = gpio_request_output(rollsh->gpio_up, rollsh->gpio_up_name);
  if (rollsh->gpio_up_line == NULL) {
    return -1;
  }

  asprintf((char **) &rollsh->gpio_down_name, "piio.rsh.%s.down", rollsh->name);
  rollsh->gpio_down_line = gpio_request_output(rollsh->gpio_down, rollsh->gpio_down_name);
  if (rollsh->gpio_down_line == NULL) {
    return -1;
  }

  return 0;
}

void rollsh_cleanup(ROLLSH_DATA_T *rollsh) {
  free((void *) rollsh->name);
  free((void *) rollsh->cmd_topic);
  free((void *) rollsh->state_topic);
  free((void *) rollsh->gpio_up_name);
  free((void *) rollsh->gpio_down_name);

  if (rollsh->gpio_up_line != NULL) {
    gpiod_line_release(rollsh->gpio_up_line);
  }

  if (rollsh->gpio_down_line != NULL) {
    gpiod_line_release(rollsh->gpio_down_line);
  }
}

void rollsh_period(ROLLSH_DATA_T *rollsh) {

}

