P1 = redissms_sender
P2 = redissms_receiver
OBJS = main.o hiredis_helper.o
CFLAGS = -g -Wall -Wstrict-prototypes
LDLIBS = -lb -lhiredis

all: $(P1) $(P2)

$(P1): $(OBJS)
	$(CC) -o $(P1) $(LDFLAGS) $(OBJS) $(LDLIBS)

$(P2): $(OBJS)
	$(CC) -o $(P2) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f *o; rm -f $(P1); rm -f $(P2)
