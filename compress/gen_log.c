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
#define SIZE_T_MAX 20

int main(int argc, char **argv)
{
	size_t msg_len = 0;
	char message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	char msg_buffer[SIZE_T_MAX+1] = { 0 };
	size_t n = 0;
	
	while ((n=read(0, msg_buffer, SIZE_T_MAX+1))) {
		//fprintf(stderr, "gen_log n:%d\n", n);
		msg_len = atol(msg_buffer);
		write(1, msg_buffer, SIZE_T_MAX+1);
           	read(0, message, msg_len);
            	write(1, message, msg_len);
	}
	
	return 0;
}
