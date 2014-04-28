
CFLAGS := -g -Wall -O2
LIBS := -lusb-1.0

all: jtag i2c wri2c rdi2c

jtag.c: jtag.h
jtag-mpsse.c: jtag.h

JTAG2232_OBJS := jtag.o jtag-mpsse.o
jtag: $(JTAG2232_OBJS)
	$(CC) -o jtag $(JTAG2232_OBJS) $(LIBS)

i2c: i2c.o jtag-mpsse.o
	$(CC) -o i2c i2c.o jtag-mpsse.o $(LIBS)

WRI2C_OBJS := wri2c.o i2c_core.o jtag-mpsse.o
wri2c: $(WRI2C_OBJS)
	$(CC) -o wri2c $(WRI2C_OBJS) $(LIBS)

RDI2C_OBJS := rdi2c.o i2c_core.o jtag-mpsse.o
rdi2c: $(RDI2C_OBJS)
	$(CC) -o rdi2c $(RDI2C_OBJS) $(LIBS)

clean::
	rm -f jtag *.o i2c wri2c rdi2c
