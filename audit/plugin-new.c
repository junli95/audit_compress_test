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


static volatile int stop = 0;
static volatile int hup = 0;

pid_t mypid = -1;
int audit_fd = -1;

struct audit_rule_data *rules = NULL;

#define SIZE_T_MAX 20
#define MQTT_MSG_LEN_MAX ( MAX_AUDIT_MESSAGE_LENGTH + 128 )

static void term_handler(int sig)
{
	stop = 1;
}

static void hup_handler(int sig)
{
	hup = 1;
}

static void set_signal_handler(void)
{
	struct sigaction sa;

	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	
	sa.sa_handler = term_handler;
	sigaction(SIGTERM, &sa, NULL);
	sa.sa_handler = hup_handler;
	sigaction(SIGHUP, &sa, NULL);
}

int main(int argc, char **argv)
{
	size_t  msg_len=0;
	char message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	char msg_buffer[SIZE_T_MAX+1] = { 0 };
	audit_fd = open("/mnt/audit_compress_test/audit/audit.log",O_CREAT|O_RDWR,0666);

	if(audit_fd==-1)
	{
		fprintf(stderr, "the output file can't open\n");
	}

	set_signal_handler();	

	do{
		if ((msg_len = read(0, message, MAX_AUDIT_MESSAGE_LENGTH))) {
			
			snprintf(msg_buffer, SIZE_T_MAX+1,"%lu", msg_len);
			if(-1==write(audit_fd, msg_buffer, SIZE_T_MAX+1)){
				fprintf(stderr, "write error, msg_len:%lu\n", msg_len);		
				
			}
			if(-1==write(audit_fd, message, msg_len)){
				fprintf(stderr, "write error, message:%s\n",message);
				
			}
			//fprintf(stderr, "message\n%s\n",message);
		}
	} while (!hup && !stop);
	
	return 0;
}
