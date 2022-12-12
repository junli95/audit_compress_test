#
#
#ARCH=x86_64
#ARCH=aarch64
#
ARCH ?= $(shell arch)
include mkdef.$(ARCH)
LIB=/usr/local/lib
INCLUDES=-I../audit-userspace/ -I../audit-userspace/auparse/ -I../audit-userspace/lib/ -I../audit-userspace/src/ -I/usr/local/include -I/home/junli/lz4/lib/ -I/home/junli/zstd/lib/ -I/usr/local/include/


all: plugin-lz4 plugin-zstd

gen_log: gen_log.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
compress: compress.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
plugin-lz4: plugin-lz4.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto -llz4
	sudo cp -f $@ /usr/local/sbin/plugin-new
plugin-zstd: plugin-zstd.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto -lzstd
	sudo cp -f $@ /usr/local/sbin/plugin-new
clean:
	rm -f *.o gen_log compress plugin-new *.compress *.log plugin-compress plugin-lz4 plugin-zstd *.decompress 
