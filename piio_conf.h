#ifndef _PIIO_CONF_H_
#define _PIIO_CONF_H_

#include <confuse.h>

typedef int (* PIIO_CONF_CONFIG_CHILD_CB)(cfg_t *cfg, void *ctx, void *child);

int piio_conf_load(const char *file);
void piio_conf_cleanup(void);

int piio_conf_config_childs(cfg_t *cfg, const char *name, int *count, void **data, int size, void *ctx, PIIO_CONF_CONFIG_CHILD_CB cccb);

char *piio_conf_strdup(const char *s);

#endif

