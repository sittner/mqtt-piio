#include "piio_conf.h"

#include "mqtt.h"

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

static cfg_opt_t rollsh_opts[] = {
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
  CFG_SEC("rollershutter", rollsh_opts, CFGF_MULTI | CFGF_TITLE),
  CFG_END()
};

SWITCH_DATA_T *piio_conf_switch;
int piio_conf_switch_count;

ROLLSH_DATA_T *piio_conf_rollsh;
int piio_conf_rollsh_count;

int piio_conf_load(const char *file) {
  cfg_t *cfg;
  cfg_t *mqtt_cfg;
  SWITCH_DATA_T *sw;
  ROLLSH_DATA_T *rollsh;
  int i;
  int err;

  memset(&mqtt_conf, 0, sizeof(mqtt_conf));
  piio_conf_switch = NULL;
  piio_conf_switch_count = 0;
  piio_conf_rollsh = NULL;
  piio_conf_rollsh_count = 0;

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

  mqtt_configure(mqtt_cfg);

  piio_conf_switch_count = cfg_size(cfg, "switch");
  piio_conf_switch = calloc(piio_conf_switch_count, sizeof(SWITCH_DATA_T));
  for (i = 0, sw = piio_conf_switch; i < piio_conf_switch_count; i++, sw++) {
    if (switch_init(cfg_getnsec(cfg, "switch", i), sw) < 0) {
      goto fail2;
    }
  }

  piio_conf_rollsh_count = cfg_size(cfg, "rollershutter");
  piio_conf_rollsh = calloc(piio_conf_rollsh_count, sizeof(ROLLSH_DATA_T));
  for (i = 0, rollsh = piio_conf_rollsh; i < piio_conf_rollsh_count; i++, rollsh++) {
    if (rollsh_init(cfg_getnsec(cfg, "rollshutter", i), rollsh) < 0) {
      goto fail2;
    }
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
  ROLLSH_DATA_T *rollsh;
  int i;

  for (i = 0, rollsh = piio_conf_rollsh; i < piio_conf_rollsh_count; i++, rollsh++) {
    rollsh_cleanup(rollsh);
  }
  free((void *) piio_conf_rollsh);

  for (i = 0, sw = piio_conf_switch; i < piio_conf_switch_count; i++, sw++) {
    switch_cleanup(sw);
  }
  free((void *) piio_conf_rollsh);

  mqtt_unconfigure();
}

