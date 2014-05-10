CC = gcc
CFLAGS += -Wall -std=c99 -pedantic
LDFLAGS +=

HDR = *.h
DST = yaskke
SRC = yaskke.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
