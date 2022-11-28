#
#
#ARCH=x86_64
#ARCH=aarch64
#
ARCH ?= $(shell arch)
include mkdef.$(ARCH)
LIB=/usr/local/lib
INCLUDES=-I/home/junli/lz4/lib/ -I/home/junli/zstd/lib/ -I/usr/local/include/

all: gen-log compress-lz4 compress-zstd

gen-log: gen_log.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) 
compress-zstd: compress_zstd.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lzstd
compress-lz4: compress_lz4.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -llz4
plugin: plugin-new.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
	sudo cp -f $@ /usr/local/sbin
clean:
	rm -f *.o gen-log compress-lz4 compress-zstd plugin *.compress *.decompress
	
