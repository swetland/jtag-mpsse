
CFLAGS := -g -Wall -O2
LIBS := -lusb-1.0

all: jtag i2c

jtag.c: jtag.h
jtag-mpsse.c: jtag.h

JTAG2232_OBJS := jtag.o jtag-mpsse.o
jtag: $(JTAG2232_OBJS)
	$(CC) -o jtag $(JTAG2232_OBJS) $(LIBS)

i2c: i2c.o jtag-mpsse.o
	$(CC) -o i2c i2c.o jtag-mpsse.o $(LIBS)

clean::
	rm -f jtag *.o
