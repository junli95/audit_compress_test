#
#
#ARCH=x86_64
#ARCH=aarch64
#
ARCH ?= $(shell arch)
include mkdef.$(ARCH)
LIB=/usr/local/lib
INCLUDES=-I../audit-userspace/ -I../audit-userspace/auparse/ -I../audit-userspace/lib/ -I../audit-userspace/src/ -I/usr/local/include

all: plugin-new

gen_log: gen_log.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
compress: compress.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
plugin-new: plugin-new.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
	sudo cp -f $@ /usr/local/sbin
clean:
	rm -f *.o gen_log compress plugin-new *.compress *.log
