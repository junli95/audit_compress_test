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
	audit_fd = open("/mnt/audit_log/audit.log.compress",O_CREAT|O_RDWR,0666);
	//size_t n = 0;

		while ((msg_len = read(0, message, MAX_AUDIT_MESSAGE_LENGTH))) {
			//fprintf(stderr, "msg_len = %ld\n", msg_len);
            		
			//write -> compress function
            		write(audit_fd, message, msg_len);
		}


	return 0;
}
