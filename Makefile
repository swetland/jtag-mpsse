
CFLAGS := -g -Wall -O2
LIBS := -lusb-1.0

all: jtag

jtag.c: jtag.h
jtag-mpsse.c: jtag.h

JTAG2232_OBJS := jtag.o jtag-mpsse.o
jtag: $(JTAG2232_OBJS)
	$(CC) -o jtag $(JTAG2232_OBJS) $(LIBS)

clean::
	rm -f jtag *.o
