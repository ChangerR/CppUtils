CC?=gcc
CFLAGS = -Wall -g
#OBJS= client.o server.o proxy.o
OBJS = proxy.o server.o client.o
DEPS=$(addsuffix .d,$(basename $(OBJS)))
LINK_LIBS=-luv

all: proxy

proxy:$(OBJS)
	$(CC) -o $@ $(OBJS) $(LINK_LIBS)

%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.d : %.c
	@$(CC) -MM $(CFLAGS) $< > $@

clean:
	rm -f $(OBJS) $(DEPS)

-include $(DEPS)