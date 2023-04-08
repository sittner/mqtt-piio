#ifndef _PIIO_CONF_H_
#define _PIIO_CONF_H_

#include "mqtt.h"
#include "switch.h"
#include "rollershutter.h"

extern const MQTT_CONF_DATA_T *piio_conf_mqtt;

extern SWITCH_DATA_T *piio_conf_switch;
extern int piio_conf_switch_count;

extern ROLLSH_DATA_T *piio_conf_rollersh;
extern int piio_conf_rollersh_count;

int piio_conf_load(const char *file);
void piio_conf_cleanup(void);

#endif

