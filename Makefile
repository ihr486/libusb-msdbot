CSRCS := $(wildcard *.c)
COBJS := $(CSRCS:%.c=%.o)
EXE := sample
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -g $(shell pkg-config --cflags libusb-1.0)
LDFLAGS := $(shell pkg-config --libs libusb-1.0)
.PHONY: all clean
all: $(EXE)
clean:
	-@rm -vf $(EXE) $(COBJS)
$(EXE): $(COBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
