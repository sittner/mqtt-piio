#include "mqtt.h"
#include "piio_conf.h"
#include "switch.h"
#include "rollershutter.h"
#include "piio_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <mosquitto.h>

#define KEEPALIVE_PERIOD 300

#define CONST_STR_PAYLOAD(s) (sizeof(s) - 1), s

MQTT_CONF_DATA_T mqtt_conf;

static struct mosquitto *mosq = NULL;

static void connect_callback(struct mosquitto *mosq, void *obj, int result) {
  mosquitto_publish(mosq, NULL, mqtt_conf.status_topic, CONST_STR_PAYLOAD("ON"), 1, true);

  // subscribe switches
  switch_mqtt_subscribe(mosq);

  // subscribe rollershutters
  rollsh_mqtt_subscribe(mosq);
}

static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
  char buf[32];

  // check for maximum payload length
  if (msg->payloadlen >= (sizeof(buf) - 1)) {
    return;
  }

  // get payload as string
  memcpy(buf, msg->payload, msg->payloadlen);
  buf[msg->payloadlen] = 0;

  // process switches
  switch_mqtt_msg(msg->topic, buf);

  // process rollershutters
  rollsh_mqtt_msg(msg->topic, buf);
}

void mqtt_init(void) {
  memset(&mqtt_conf, 0, sizeof(mqtt_conf));
}

int mqtt_configure(cfg_t *cfg) {
  cfg_t *mqtt_cfg;

  mqtt_cfg = cfg_getsec(cfg, "mqtt");
  if (mqtt_cfg == NULL) {
    syslog(LOG_ERR, "ERROR: no mqtt config found.");
    return -1;
  }

  mqtt_conf.base_topic = piio_conf_strdup(cfg_getstr(mqtt_cfg, "topic"));
  mqtt_conf.host = piio_conf_strdup(cfg_getstr(mqtt_cfg, "host"));
  mqtt_conf.port = cfg_getint(mqtt_cfg, "port");
  mqtt_conf.client_id = piio_conf_strdup(cfg_getstr(mqtt_cfg, "client_id"));
  mqtt_conf.username = piio_conf_strdup(cfg_getstr(mqtt_cfg, "user"));
  mqtt_conf.password = piio_conf_strdup(cfg_getstr(mqtt_cfg, "passwd"));

  asprintf((char **) &mqtt_conf.status_topic, "%s/status", mqtt_conf.base_topic);

  return 0;
}

void mqtt_unconfigure(void) {
  free((void *) mqtt_conf.base_topic);
  free((void *) mqtt_conf.host);
  free((void *) mqtt_conf.client_id);
  free((void *) mqtt_conf.username);
  free((void *) mqtt_conf.password);
  free((void *) mqtt_conf.status_topic);
}

int mqtt_startup(void) {
  if (mosquitto_lib_init()) {
    syslog(LOG_ERR, "Failed to init mosquitto lib");
    goto fail0;
  }

  mosq = mosquitto_new(mqtt_conf.client_id, true, NULL);
  if (mosq == NULL) {
    syslog(LOG_ERR, "Failed to create mosquitto instance");
    goto fail1;
  }

  if (mqtt_conf.username != NULL) {
    if (mosquitto_username_pw_set(mosq, mqtt_conf.username, mqtt_conf.password)) {
      syslog(LOG_ERR, "Failed to set mosquitto credentials");
      goto fail2;
    }
  }

  mosquitto_connect_callback_set(mosq, connect_callback);
  mosquitto_message_callback_set(mosq, message_callback);

  if (mosquitto_will_set(mosq, mqtt_conf.status_topic, CONST_STR_PAYLOAD("OFF"), 1, true)) {
    syslog(LOG_ERR, "Failed to set mosquitto will");
    goto fail2;
  }

  if (mosquitto_loop_start(mosq)) {
    syslog(LOG_ERR, "Failed to start mosquitto thread");
    goto fail3;
  }

  if (mosquitto_connect_async(mosq, mqtt_conf.host, mqtt_conf.port, KEEPALIVE_PERIOD)) {
    syslog(LOG_INFO, "initial mqtt connection failed");
  }

  return 0;

fail3:
  mosquitto_disconnect(mosq);
fail2:
  mosquitto_destroy(mosq);
fail1:
  mosquitto_lib_cleanup();
fail0:
  return -1;
}

void mqtt_shutdown(void) {
  mosquitto_publish(mosq, NULL, mqtt_conf.status_topic, CONST_STR_PAYLOAD("OFF"), 1, true);
  mosquitto_disconnect(mosq);
  mosquitto_loop_stop(mosq, false);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
}

int mqtt_publish_switch(const char *topic, int val) {
  if (val) {
    return mosquitto_publish(mosq, NULL, topic, CONST_STR_PAYLOAD("ON"), 1, true);
  } else {
    return mosquitto_publish(mosq, NULL, topic, CONST_STR_PAYLOAD("OFF"), 1, true);
  }
}

int mqtt_publish_int(const char *topic, int val) {
  char buf[32];
  int len;

  len = snprintf(buf, sizeof(buf), "%d", val);
  if (len > sizeof(buf)) {
    return MOSQ_ERR_PAYLOAD_SIZE;
  }

  return mosquitto_publish(mosq, NULL, topic, len, buf, 1, true);
}

