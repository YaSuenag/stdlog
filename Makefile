.PHONY: clean

CC ?= gcc
CPPFLAGS += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux \
            -D_GNU_SOURCE -D_LARGEFILE64_SOURCE
CFLAGS += -c -g -fPIC -O3
LDFLAGS += -lpthread

TARGET = libstdlog.so
OBJS = main.o linux/pd_linux.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJS)

