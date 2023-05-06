CC ?= gcc

TARGET = piio

SRC = \
	main.c \
	piio_conf.c \
	timer.c \
	mqtt.c \
	switch.c \
	rollershutter.c \

OBJ = $(SRC:.c=.o)

CFLAGS += -DGCC_COMPILER -D_GNU_SOURCE

CFLAGS += -I.

CFLAGS += -Wall

LIBS += -lconfuse -lmosquitto

.PHONY: all clean realclean install

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $< 

clean:
	rm -f $(OBJ)
	rm -f $(TARGET)

install: $(TARGET)
	install -m755 -D $(TARGET) $(DESTDIR)/usr/bin/$(TARGET)

realclean: clean
	make -C test clean

