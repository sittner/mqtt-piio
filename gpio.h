#ifndef _GPIO_H
#define _GPIO_H

#include <gpiod.h>

int gpio_init(void);
void gpio_cleanup(void);

struct gpiod_line *gpio_request_output(int gpio, const char *name);

#endif

