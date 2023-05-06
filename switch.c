#include "switch.h"
#include "mqtt.h"
#include "gpio.h"

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int switch_init(cfg_t *cfg, SWITCH_DATA_T *sw) {
  sw->name = strdup(cfg_title(cfg));
  sw->gpio = cfg_getint(cfg, "gpio");
  sw->on_time = cfg_getint(cfg, "on_time");

  asprintf((char **) &sw->cmd_topic, "%s/%s/cmd", mqtt_conf.base_topic, sw->name);
  asprintf((char **) &sw->state_topic, "%s/%s/state", mqtt_conf.base_topic, sw->name);

  asprintf((char **) &sw->gpio_name, "piio.sw.%s", sw->name);
  sw->gpio_line = gpio_request_output(sw->gpio, sw->gpio_name);
  if (sw->gpio_line == NULL) {
    return -1;
  }

  return 0;
}

void switch_cleanup(SWITCH_DATA_T *sw) {
  free((void *) sw->name);
  free((void *) sw->cmd_topic);
  free((void *) sw->state_topic);
  free((void *) sw->gpio_name);

  if (sw->gpio_line != NULL) {
    gpiod_line_release(sw->gpio_line);
  }
}

void switch_period(SWITCH_DATA_T *sw) {

}

