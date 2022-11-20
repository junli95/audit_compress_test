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

#define MQTT_MSG_LEN_MAX ( MAX_AUDIT_MESSAGE_LENGTH + 128 )

static void handle_event(auparse_state_t *au, auparse_cb_event_t cb_event_type, void *user_data)
{
	int nrec = 0, tmp_len = 0;
	char msg_buf[MQTT_MSG_LEN_MAX] = { 0 };
	//struct mqtt_message_node *node = NULL;

	if (cb_event_type != AUPARSE_CB_EVENT_READY)
		return;
	
	while (auparse_goto_record_num(au, nrec++) > 0) {
		int type;
		const char *p_field = NULL;

		type = auparse_get_type(au);

		if (type == AUDIT_SYSCALL) {
			int pid;
			
			p_field = auparse_find_field(au, "pid");
			if (!p_field) {
				fprintf(stderr, "[WARNING] Unable to find pid\n");
				continue;
			}

			pid = atoi(auparse_get_field_str(au));
			auparse_first_field(au);
/*
			p_field = auparse_find_field(au, "syscall");
			if (!p_field) { 
				fprintf(stderr, "[WARNING] Unable to find syscall\n");
				continue;
			}

			syscall = atoi(auparse_get_field_str(au));
			auparse_first_field(au);
*/
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

	//node = mqtt_new_message(msg_buf, tmp_len, 1, "/audit/camera");
	///if (node) mqtt_publish(mqtt_inst, node);

//	fprintf(stderr, "%s\n", msg_buf);
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


int main(int argc, char **argv)
{
	int rc = 0;
	size_t  msg_len=0;
	char message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	//char msg_buffer[0xffffffff] = { 0 };
	char msg_buffer[sizeof(size_t)*8] = { 0 };
	char msg_test[sizeof(size_t)*8] = { 0 };
	//fprintf(stderr, "size of size_t:%u\n",sizeof(size_t));
	//audit_fd = open("/mnt/audit_compress_test/compress/audit.log",O_CREAT|O_RDWR,0666);
	audit_fd = open("/mnt/audit_compress_test/audit/audit.log",O_CREAT|O_RDWR,0666);
	auparse_state_t *au = NULL;

	if(audit_fd==-1)
	{
		fprintf(stderr, "the output file can't open\n");
	}
	
	mypid = getpid();
	fprintf(stderr, "Plugin PID: %d\n", mypid);

	set_signal_handler();

	//rc = init_auparselib(&au);
	//if (rc) {
	//	fprintf(stderr, "Failed to initialize auparselib\n");
	//	exit(-1);
	//}

	do{
		//if (auparse_feed_has_data(au)) {
		//	// check events for complete based on time
		//	// if there's data
		//	auparse_feed_age_events(au);
		//}
		if ((msg_len = read(0, message, MAX_AUDIT_MESSAGE_LENGTH))) {
			
			//fprintf(stderr, "**msg_len = %lu\n", msg_len);
			//fprintf(stderr, "**message = %s",message);
			snprintf(msg_buffer, sizeof(msg_len)*8,"%lu", msg_len);
			//snprintf(msg_test, sizeof(msg_test), "write_test\n");
			if(-1==write(audit_fd, msg_buffer, sizeof(msg_len)*8)){
				fprintf(stderr, "write error, msg_len:%lu\n", msg_len);		
				
			}
			if(-1==write(audit_fd, message, msg_len)){
				fprintf(stderr, "write error, message:%s\n",message);
				
			}
			//if(-1==write(audit_fd, msg_test, sizeof(msg_test))){
			//	fprintf(stderr, "write error, msg_test");

			//}
			
			//fprintf(stderr, "msg_len:%d\n",msg_len);
			//fprintf(stderr, "message\n%s\n",message);
		}
	} while (!hup && !stop);
	
	//auparse_flush_feed(au);
	//auparse_destroy(au);
	//finalize_audit(audit_fd);

	return 0;
}
