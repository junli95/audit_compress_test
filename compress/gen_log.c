#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libaudit.h>
#include <auparse.h>
#include <getopt.h>
#include <pthread.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

int audit_fd = -1;

int main(int argc, char **argv)
{
	size_t msg_len = 0;
	char message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	char msg_buffer[sizeof(size_t)] = { 0 };
	//audit_fd = open("/mnt/audit_log/audit.log",O_CREAT|O_RDWR,0666);
	size_t n = 0;
/*
	read(0, &msg_len, sizeof(size_t));
	fprintf(stderr, "msg_len = %lu\n", msg_len);
	read(0, message, msg_len);
	fprintf(stderr, "print message:%s\n",message);
*/
	while ((n=read(0, msg_buffer, sizeof(size_t)))) {
		fprintf(stderr, "n:%lu\n", n);
		msg_len = atoi(msg_buffer);
		fprintf(stderr, "msg_len = %lu\n", msg_len);
           	read(0, message, msg_len);
		fprintf(stderr, "print message:%s\n",message);
            	write(1, message, msg_len);
	}
	//fprintf(stderr, "n:%lu\n", n);
	

	return 0;
}
