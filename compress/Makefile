all: gen_log compress

gen_log: gen_log.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
compress: compress.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
plugin: plugin-new.c
	$(CC) $(INCLUDES) -Wall -g -O2 -o $@ $^ -L$(LIB) -lauparse -laudit -lmosquitto
	sudo cp -f $@ /usr/local/sbin
clean:
	rm -f *.o gen_log compress plugin *.compress
