#include "piio_conf.h"

#include <confuse.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

static cfg_opt_t mqtt_opts[] = {
  CFG_STR("topic", "piio", CFGF_NONE),
  CFG_STR("host", "localhost", CFGF_NONE),
  CFG_INT("port", 1883, CFGF_NONE),
  CFG_STR("client_id", NULL, CFGF_NONE),
  CFG_STR("user", NULL, CFGF_NONE),
  CFG_STR("passwd", NULL, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t switch_opts[] = {
  CFG_INT("gpio", 1, CFGF_NONE),
  CFG_INT("on_time", 0, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t rollershutter_opts[] = {
  CFG_INT("gpio_up", 1, CFGF_NONE),
  CFG_INT("gpio_down", 2, CFGF_NONE),
  CFG_INT("time_full", 10000, CFGF_NONE),
  CFG_INT("time_extra", 1000, CFGF_NONE),
  CFG_INT("time_pause", 1000, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t opts[] = {
  CFG_SEC("mqtt", mqtt_opts, CFGF_NONE),
  CFG_SEC("switch", switch_opts, CFGF_MULTI | CFGF_TITLE),
  CFG_SEC("rollershutter", rollershutter_opts, CFGF_MULTI | CFGF_TITLE),
  CFG_END()
};

const MQTT_CONF_DATA_T *piio_conf_mqtt;

SWITCH_DATA_T *piio_conf_switch;
int piio_conf_switch_count;

ROLLSH_DATA_T *piio_conf_rollersh;
int piio_conf_rollersh_count;

static char *strdup_null(const char *s) {
  if (s == NULL) {
    return NULL;
  }
  return strdup(s);
}

int piio_conf_load(const char *file) {
  cfg_t *cfg;
  cfg_t *mqtt_cfg;
  MQTT_CONF_DATA_T *mqtt;
  cfg_t *sw_cfg;
  SWITCH_DATA_T *sw;
  cfg_t *rollersh_cfg;
  ROLLSH_DATA_T *rollersh;
  int i;
  int err;

  piio_conf_mqtt = NULL;
  piio_conf_switch = NULL;
  piio_conf_switch_count = 0;
  piio_conf_rollersh = NULL;
  piio_conf_rollersh_count = 0;

  cfg = cfg_init(opts, CFGF_NOCASE);
  if (cfg == NULL) {
    syslog(LOG_ERR, "ERROR: failed to create config file parser.");
    goto fail0;
  }

  err = cfg_parse(cfg, file);
  if (err == CFG_FILE_ERROR) {
    syslog(LOG_ERR, "ERROR: failed to open config file %s.", file);
    goto fail1;
  }
  if (err == CFG_PARSE_ERROR) {
    syslog(LOG_ERR, "ERROR: failed to parse config file %s.", file);
    goto fail1;
  }
  if (err != CFG_SUCCESS) {
    syslog(LOG_ERR, "ERROR: unknown error on loading config file %s.", file);
    goto fail1;
  }

  mqtt_cfg = cfg_getsec(cfg, "mqtt");
  if (mqtt_cfg == NULL) {
    syslog(LOG_ERR, "ERROR: no mqtt config found in %s.", file);
    goto fail2;
  }

  mqtt = calloc(1, sizeof(MQTT_CONF_DATA_T));
  mqtt->topic = strdup_null(cfg_getstr(mqtt_cfg, "topic"));
  mqtt->host = strdup_null(cfg_getstr(mqtt_cfg, "host"));
  mqtt->port = cfg_getint(mqtt_cfg, "port");
  mqtt->client_id = strdup_null(cfg_getstr(mqtt_cfg, "client_id"));
  mqtt->username = strdup_null(cfg_getstr(mqtt_cfg, "user"));
  mqtt->password = strdup_null(cfg_getstr(mqtt_cfg, "passwd"));
  piio_conf_mqtt = mqtt;

  piio_conf_switch_count = cfg_size(cfg, "switch");
  piio_conf_switch = calloc(piio_conf_switch_count, sizeof(SWITCH_DATA_T));
  for (i = 0, sw = piio_conf_switch; i < piio_conf_switch_count; i++, sw++) {
    sw_cfg = cfg_getnsec(cfg, "switch", i);
    sw->name = strdup(cfg_title(sw_cfg));
    sw->gpio = cfg_getint(sw_cfg, "gpio");
    sw->on_time = cfg_getint(sw_cfg, "on_time");
  }

  piio_conf_rollersh_count = cfg_size(cfg, "rollershutter");
  piio_conf_rollersh = calloc(piio_conf_rollersh_count, sizeof(ROLLSH_DATA_T));
  for (i = 0, rollersh = piio_conf_rollersh; i < piio_conf_rollersh_count; i++, rollersh++) {
    rollersh_cfg = cfg_getnsec(cfg, "rollershutter", i);
    rollersh->name = strdup(cfg_title(rollersh_cfg));
    rollersh->gpio_up = cfg_getint(rollersh_cfg, "gpio_up");
    rollersh->gpio_down = cfg_getint(rollersh_cfg, "gpio_down");
    rollersh->time_full = cfg_getint(rollersh_cfg, "time_full");
    rollersh->time_extra = cfg_getint(rollersh_cfg, "time_extra");
    rollersh->time_pause = cfg_getint(rollersh_cfg, "time_pause");
  }

  cfg_free(cfg);
  return 0;

fail2:
  piio_conf_cleanup();
fail1:
  cfg_free(cfg);
fail0:
  return -1;
}

void piio_conf_cleanup(void) {
  SWITCH_DATA_T *sw;
  ROLLSH_DATA_T *rollersh;
  int i;

  for (i = 0, rollersh = piio_conf_rollersh; i < piio_conf_rollersh_count; i++, rollersh++) {
    free((void *) rollersh->name);
  }
  free((void *) piio_conf_rollersh);
  piio_conf_rollersh = NULL;
  piio_conf_rollersh_count = 0;

  for (i = 0, sw = piio_conf_switch; i < piio_conf_switch_count; i++, sw++) {
    free((void *) sw->name);
  }
  free((void *) piio_conf_rollersh);
  piio_conf_switch = NULL;
  piio_conf_switch_count = 0;

  if (piio_conf_mqtt != NULL) {
    free((void *) piio_conf_mqtt->topic);
    free((void *) piio_conf_mqtt->host);
    free((void *) piio_conf_mqtt->client_id);
    free((void *) piio_conf_mqtt->username);
    free((void *) piio_conf_mqtt->password);
  }
  free((void *) piio_conf_mqtt);
  piio_conf_mqtt = NULL;
}

