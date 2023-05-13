#include "piio_conf.h"

#include "mqtt.h"
#include "switch.h"
#include "rollershutter.h"

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
  CFG_INT("pin", 1, CFGF_NONE),
  CFG_INT("on_time", 0, CFGF_NONE),
  CFG_END()
};

static cfg_opt_t rollsh_opts[] = {
  CFG_INT("pin_up", 1, CFGF_NONE),
  CFG_INT("pin_down", 2, CFGF_NONE),
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

int piio_conf_load(const char *file) {
  cfg_t *cfg;
  int err;

  mqtt_init();
  switch_init();
  rollsh_init();

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

  if (mqtt_configure(cfg) < 0) {
    goto fail2;
  }

  if (switch_configure(cfg) < 0) {
    goto fail2;
  }

  if (rollsh_configure(cfg) < 0) {
    goto fail2;
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
  rollsh_unconfigure();
  switch_unconfigure();
  mqtt_unconfigure();
}

int piio_conf_config_childs(cfg_t *cfg, const char *name, int *count, void **data, int size, void *ctx, PIIO_CONF_CONFIG_CHILD_CB cccb) {
  int n, i;
  void *child;

  *count = 0;
  *data = NULL;

  n = cfg_size(cfg, name);
  if (n == 0) {
    return 0;
  }

  child = calloc(n, size);
  if (child == NULL) {
    syslog(LOG_ERR, "Failed to allocate child's data.");
    return -1;
  }

  *count = n;
  *data = child;

  for (i = 0; i < n; i++, child += size) {
    if (cccb(cfg_getnsec(cfg, name, i), ctx, child) < 0) {
      return -1;
    }
  }

  return 0;
}

char *piio_conf_strdup(const char *s) {
  if (s == NULL) {
    return NULL;
  }

  return strdup(s);
}

