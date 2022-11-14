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

//#include "mqtt_testlib.h"

static volatile int stop = 0;
static volatile int hup = 0;

pid_t mypid = -1;
int audit_fd = -1;
void *mqtt_inst = NULL;
struct audit_rule_data *rules = NULL;

#if 0
static int probe_param_values(int pid, unsigned long addr10, size_t sz, void *retval)
{
	int rc = 0, fd = -1;
	off_t offset = 0;
	char procmem_path[PATH_MAX] = { 0 };

	assert(retval != NULL);
	
	rc = snprintf(procmem_path, PATH_MAX, "/proc/%d/mem", pid);
	if (rc <= 0) {
		perror("Failed to generate mem filepath: ");
		goto err;
	}

	fd = open(procmem_path, O_RDONLY);
	if (fd < 0) {
		perror("open failed:");
		goto err;
	}
	
	offset = lseek(fd, addr10, SEEK_SET);
	if (offset == -1) {
		perror("lseek failed: ");
		goto err;
	}

	rc = read(fd, retval, sz);
	if (rc == -1) {
		perror("read from /proc/<pid>/mem failed: ");
		goto err;
	}

	if (fd != -1) close(fd);

	return 0;

err:
	fprintf(stderr, "Failed to probe syscall parameter values\n");
	return errno;
}
#endif 

#define MQTT_MSG_LEN_MAX ( MAX_AUDIT_MESSAGE_LENGTH + 128 )

static void handle_event(auparse_state_t *au, auparse_cb_event_t cb_event_type, void *user_data)
{
	int nrec = 0, tmp_len = 0;
	char msg_buf[MQTT_MSG_LEN_MAX] = { 0 };
	//struct mqtt_message_node *node = NULL;

	if (cb_event_type != AUPARSE_CB_EVENT_READY)
		return;
/*	
	while (auparse_goto_record_num(au, nrec++) > 0) {
		int type;
		const char *p_field = NULL;

		type = auparse_get_type(au);

		if (type == AUDIT_SYSCALL) {
			int pid, syscall;
			
			p_field = auparse_find_field(au, "pid");
			if (!p_field) {
				fprintf(stderr, "[WARNING] Unable to find pid\n");
				continue;
			}

			pid = atoi(auparse_get_field_str(au));
			auparse_first_field(au);

			tmp_len += snprintf(msg_buf + tmp_len, MQTT_MSG_LEN_MAX,
				"Message from %d:\t%s\t", pid, auparse_get_record_text(au));
		}
		else if (type == AUDIT_PATH) {
			p_field = auparse_find_field(au, "name");
			if (!p_field) {
				fprintf(stderr, "[WARNING] Unable to find name\n");
				continue;
			}

			tmp_len += snprintf(msg_buf + tmp_len, MQTT_MSG_LEN_MAX, "Opened filename: %s\t",
				auparse_get_field_str(au));
		}
		else continue;
	}
*/	
	//**change to compresser**
	//
	//node = mqtt_new_message(msg_buf, tmp_len, 1, "/audit/camera");
	//if (node) mqtt_publish(mqtt_inst, node);

//	fprintf(stderr, "%s\n", msg_buf);
}

static int audit_rules_preset(pid_t *pid_list, int list_len)
{
	int fd = 0, rc = 0;
	int flags = AUDIT_FILTER_EXIT;
	

	fd = audit_open();
	if (fd < 0) {
		perror("audit_open failed: ");
		goto err3;
	}

	/* The man page of audit_rule_create_data() is not written.
	 * It allocates a memory area using malloc(), and thus, it should be
	 * deallocated by free() */
	rules = audit_rule_create_data();
	if (rules == NULL) {
		perror("audit_rule_create_data: ");
		goto err3;
	}

	for (int i = 0; i < list_len; i++) {
		int len = 0;
		char rule_skip[32] = { 0 };

		len = snprintf(rule_skip, 32, "pid!=%d", pid_list[i]);
		rule_skip[len] = '\0';

		rc |= audit_rule_fieldpair_data(&rules, rule_skip, flags);
	}

	if (rc < 0) {
		perror("audit_rule_fieldpair_data: ");
		goto err2;
	}
/*
	rc |= audit_rule_syscallbyname_data(rules, "openat");
	rc |= audit_rule_syscallbyname_data(rules, "ioctl");
	rc |= audit_rule_syscallbyname_data(rules, "read");
	rc |= audit_rule_syscallbyname_data(rules, "write");
*/
	rc |= audit_rule_syscallbyname_data(rules, "all");
	if (rc < 0) {
		perror("audit_rule_syscallbyname_data: ");
		goto err2;
	}
	
	rc = audit_add_rule_data(fd, rules, flags, AUDIT_ALWAYS);
	if (rc <= 0) {
		perror("audit_add_rule_data: ");
		goto err2;
	}

	return fd;
err2:
	free(rules);
err3:
	fd = -1;
	return fd;
}

static int finalize_audit(int fd)
{
	int rc = 0;

	/* delete the audit rule */
	if (fd == -1) return 0;

	rc = audit_delete_rule_data(fd, rules, AUDIT_FILTER_EXIT, AUDIT_ALWAYS);
	if (rc <= 0) {
		perror("audit_delete_rule_data fails: ");
	}

	audit_rule_free_data(rules);
	audit_close(fd);
	
	return rc;
}

static int init_auparselib(auparse_state_t **au)
{
	int rc = 0;

	auparse_state_t *_au = auparse_init(AUSOURCE_FEED, 0);
	if (!_au) {
		perror("auparse_init failed: ");
		return -1;
	}
	
	// Set the end of event timeout value
	rc = auparse_set_eoe_timeout(2);
	if (rc) {
		perror("auparse_set_eoe_timeout: ");
		return rc;
	}
	// Add a callback handler for notification
	// void auparse_add_callback(auparse_state_t *au,
	//			     auparse_callback_ptr callback,
	//		             void *user_data, 
	//			     user_destroy user_destroy_func);
	auparse_add_callback(_au, handle_event, NULL, NULL);

	*au = _au;
	return rc;
}

static void *init_mqtt(void)
{
	char *host = "localhost";
	int port = 1883;
	int keepalive = 60;

	return mqtt_init(host, port, keepalive);
}

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

#define PID_LIST_MAX_LEN 256

int parse_pids(char *args_, pid_t *pid_list)
{
	int len = 0;
	char *args = strdup(args_);
	char *tok = NULL;
	
	tok = strtok(args, ",");
	while (tok) {
		pid_list[len++] = atoi(tok);
		tok = strtok(NULL, ",");
	}
	free(args);

	return len;
}

int main(int argc, char **argv)
{
	int rc = 0, len = 1;
	auparse_state_t *au = NULL;
	//size_t msg_len = 0;
	size_t  msg_len=0;
	char message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	pid_t pid_list[PID_LIST_MAX_LEN] = { 0 };
	//char len[1] = {0};
	//char msg_buffer[0xffffffff] = { 0 };
	char msg_buffer[sizeof(size_t)] = { 0 };
	//fprintf(stderr, "size of size_t:%u\n",sizeof(size_t));
	audit_fd = open("/mnt/audit_log/audit.log",O_CREAT|O_RDWR,0666);
	//FILE * logfile = fopen("/mnt/audit_log/audit.log", "w");

	if(audit_fd==-1)
	{
		fprintf(stderr, "the output file can't open\n");
		return NULL;
	}
	//else
	//{	fprintf(stderr, "audit_fd:%d\n", audit_fd);
	//}
	//if(logfile==NULL)
	//{
	//	fprintf(stderr, "the logfilefile can't open\n");
	//	return NULL;
	//}
	
	//pid_t pid_list[PID_LIST_MAX_LEN] = { 0 };


	mypid = getpid();
	fprintf(stderr, "Plugin PID: %d\n", mypid);

	set_signal_handler();

	//audit_fd = audit_rules_preset(pid_list, len);


	do{
	//fprintf(stderr, "start write logfile\n");
		if ((msg_len = read(0, message, MAX_AUDIT_MESSAGE_LENGTH))) {
			
			fprintf(stderr, "**msg_len = %u\n **msg_len\n", msg_len);
			fprintf(stderr, "**message = \n%s\n **message\n",message);
			//fprintf(stderr, "before:char msg:%s\n",msg_buffer);
			snprintf(msg_buffer, sizeof(msg_len),"u", msg_len);
			//for(int temp_i=0;temp_i<8;temp_i++)
			//fprintf(stderr, "char msg:%s\n",msg_buffer);
			if(-1==write(audit_fd, msg_buffer, sizeof(msg_len))){
				fprintf(stderr, "write error, msg_len:%d\n", msg_len);		
				
			}
			//if(fprintf(logfile,"%d ",msg_len)<0){
			//	fprintf(stderr, "logfile write error:%d\n", msg_len);
			//}else{
			//	fflush(logfile);
			//	fprintf(stderr, "msg_len:%d\n", msg_len);
			//}
			if(-1==write(audit_fd, message, msg_len)){
				fprintf(stderr, "write error, message:%s\n",message);
				
			}
			//else{
			//	write(audit_fd, msg_buffer, sizeof(msg_len));
			//}
			
			//fprintf(stderr, "msg_len:%d\n",msg_len);
			//fprintf(stderr, "message\n%s\n",message);
			//fprintf(stderr, "2\n");
		}
	//fprintf(stderr, "3\n");
	//fprintf(stderr,"out_print\n");
	} while (!hup && !stop);
/*
	do {
		if (auparse_feed_has_data(au)) {
			// check events for complete based on time 
			// if there's data
			auparse_feed_age_events(au);
		}

		if ((msg_len = read(0, message, MAX_AUDIT_MESSAGE_LENGTH))) {
			fprintf(stderr, "msg_len = %ld\n", msg_len);
			if (auparse_feed(au, message, msg_len) < 0) {
				perror("auparse_feed failed: ");
				break;
			}
			write(logfile, &msg_len, sizeof(msg_len));
			write(logfile, message, msg_len);
			while(1)
				fprintf(stderr, "print message:\n%s\n",message);
			//write(logfile, "once\n", 20);
		}
	} while (!hup && !stop);
*/
	//auparse_flush_feed(au);
	//auparse_destroy(au);
	//if (mqtt_inst) mqtt_finalize(mqtt_inst);
	//finalize_audit(audit_fd);

	return 0;
}
