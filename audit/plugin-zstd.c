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
#include <lz4.h>
#include <lz4hc.h>
#include <zstd.h>

static volatile int stop = 0;
static volatile int hup = 0;

pid_t mypid = -1;
int audit_fd = -1;
int audit_fd_origin = -1;
static struct timeval start, end;
static struct timeval start_storage, end_storage;


struct audit_rule_data *rules = NULL;

#define SIZE_T_MAX 20
#define ChunkSize 1*1024*1024
#define dstCapacity LZ4_COMPRESSBOUND(ChunkSize)
//#define MQTT_MSG_LEN_MAX ( MAX_AUDIT_MESSAGE_LENGTH + 128 )

static float time_elapsed(struct timeval _start, struct timeval _end)
{
	return (float)(_end.tv_sec - _start.tv_sec) + (float)(_end.tv_usec - _start.tv_usec)/1000000.0f;
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

int main(int argc, char **argv)
{
	size_t  msg_len=0;
	char message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	//char msg_buffer[SIZE_T_MAX+1] = { 0 };
	audit_fd_origin = open("/mnt/audit_compress_test/audit/audit.log",O_CREAT|O_RDWR,0666);
	audit_fd = open("/mnt/audit_compress_test/audit/audit.log.compress",O_CREAT|O_RDWR,0666);
	char compress_message[dstCapacity] = { 0 };
	int compress_size = 0;
	int accumulated_size = 0;
	float compress_time = 0;
	struct stat statbuff;
	size_t decompress_size = 0;
	char compressed_size[SIZE_T_MAX+1] = { 0 };
	float storage_time = 0;
	char message_chunk[ChunkSize] = { 0 };
	size_t offset = 0;
	int compress_flag = 0;
	char temp_message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	int temp_flag = 0;
	float original_storage_time = 0;
	float decompress_time = 0;

	if(audit_fd==-1)
	{
		fprintf(stderr, "the output file can't open\n");
	}

	set_signal_handler();	

	do{
		if ((msg_len = read(0, message, MAX_AUDIT_MESSAGE_LENGTH))) {
			//fprintf(stderr, "msg_len:%lu\n",msg_len);	
			//snprintf(msg_buffer, SIZE_T_MAX+1,"%lu", msg_len);
			
			//write the original log file
			gettimeofday(&start, NULL);
			if(-1==write(audit_fd_origin, message, msg_len)){
				fprintf(stderr, "write error, message:%s\n",message);
			}
			gettimeofday(&end, NULL);
			original_storage_time += time_elapsed(start, end);

			gettimeofday(&start, NULL);

			if (msg_len<=0)
				fprintf(stderr, "msg_len:%ld\n",msg_len);
			if (temp_flag){
				strncpy(message_chunk, temp_message, offset);
				temp_flag=0;
				//fprintf(stderr, "message_chunk:%s\n", message_chunk);
				//fprintf(stderr, "exit temp\n");
			}
			accumulated_size += msg_len;

			//append into message_chunk
			if (accumulated_size >= ChunkSize){
				if(accumulated_size == ChunkSize){
					strncpy(message_chunk+offset, message, msg_len);
					offset = 0;

				}
				else{
					//strncpy(message_chunk+offset, message, ChunkSize-offset);
					//offset = msg_len - (ChunkSize-offset);
					//strncpy(temp_message, message+ChunkSize-offset, offset);
					strncpy(temp_message, message, msg_len);
					offset = msg_len;	
					temp_flag = 1;
					accumulated_size -= msg_len;
				}
				compress_flag = 1;
				//accumulated_size = offset;
			}
			else{
				strncpy(message_chunk+offset, message, msg_len);
				offset += msg_len;
			}
			//fprintf(stderr, "offset:%ld, compress_flag:%d\n", offset, compress_flag);
			//fprintf(stderr, "message_chunk:%s\n", message_chunk);

			if (compress_flag){
				//fprintf(stderr, "enter compress\n");
				if((compress_size = ZSTD_compress(compress_message, ZSTD_compressBound(accumulated_size), message_chunk, accumulated_size, 1))){

					//if(0==compress_size)
					//	fprintf(stderr, "error in compress\n");
					snprintf(compressed_size, SIZE_T_MAX+1,"%u", compress_size);
					//fprintf(stderr, "%lu\n", compress_size);
					gettimeofday(&start_storage, NULL);
					write(audit_fd, compressed_size, SIZE_T_MAX+1);
					write(audit_fd, compress_message, compress_size);
					gettimeofday(&end_storage, NULL);
					storage_time += time_elapsed(start_storage, end_storage);	
				}
				compress_flag = 0;
				accumulated_size = offset;

				//fprintf(stderr, "exit compress\n");

			}


			/*pre version
			if(-1==write(audit_fd, msg_buffer, SIZE_T_MAX+1)){
				fprintf(stderr, "write error, msg_len:%lu\n", msg_len);		
				
			}
			if(-1==write(audit_fd, message, msg_len)){
				fprintf(stderr, "write error, message:%s\n",message);
				
			}*/

			//fprintf(stderr, "message\n%s\n",message);
		}
		gettimeofday(&end, NULL);
		compress_time += time_elapsed(start, end);
	} while (!hup && !stop);
	//compress the leave contents before the program ends
	if (temp_flag){
		strncpy(message_chunk, temp_message, offset);
		temp_flag=0;
		//fprintf(stderr, "message_chunk:%s\n", message_chunk);
		//fprintf(stderr, "exit temp\n");
	}

	gettimeofday(&start, NULL);
		if((compress_size = ZSTD_compress(compress_message, ZSTD_compressBound(accumulated_size), message_chunk, accumulated_size, 1))){
		//if(0==compress_size)
		//	fprintf(stderr, "error in compress\n");
		snprintf(compressed_size, SIZE_T_MAX+1,"%u", compress_size);
		//fprintf(stderr, "%lu\n", compress_size);
		gettimeofday(&start_storage, NULL);
		write(audit_fd, compressed_size, SIZE_T_MAX+1);
		write(audit_fd, compress_message, compress_size);
		gettimeofday(&end_storage, NULL);
		storage_time += time_elapsed(start_storage, end_storage);	
	}

	gettimeofday(&end, NULL);
	compress_time += time_elapsed(start, end);
	/*gettimeofday(&start, NULL);
	if((compress_size = LZ4_compress_default(message, compress_message, accumulated_size, LZ4_compressBound(accumulated_size)))){
		write(audit_fd, compress_message, compress_size);
	}
	gettimeofday(&end, NULL);                 	
	compress_time += time_elapsed(start, end);*/
	fprintf(stderr, "original log file storage time:%12.9f sec\n", original_storage_time);
	fprintf(stderr, "Accumulated storage time:%12.9f sec\n", storage_time);
	fprintf(stderr, "Accumulated compress time:%12.9f sec\n", compress_time-storage_time);
	if(-1 == stat("/mnt/audit_compress_test/audit/audit.log", &statbuff)){
			fprintf(stderr, "stat error\n");
		}else{
			fprintf(stderr, "original log size:%ld B\n",statbuff.st_size);
		}
		if(-1 == stat("/mnt/audit_compress_test/audit/audit.log.compress", &statbuff)){
			fprintf(stderr, "stat error\n");
		}else{
			fprintf(stderr, "compressed log size:%ld B\n",statbuff.st_size);
		}


	close(audit_fd);

	//validate the function of decompression
	audit_fd = open("/mnt/audit_compress_test/audit/audit.log.compress",O_CREAT|O_RDWR,0666);
	int decompress_fd = open("/mnt/audit_compress_test/audit/audit.log.decompress",O_CREAT|O_RDWR,0666);
	
	gettimeofday(&start, NULL);
	while((msg_len=(read(audit_fd, compressed_size, SIZE_T_MAX+1)))){
		//fprintf(stderr, "n:%ld\n", n);
		compress_size = atol(compressed_size);
		//fprintf(stderr, "compress_size:%ld\n", compress_size);
		read(audit_fd, compress_message, compress_size);
		if((decompress_size = ZSTD_decompress(message, dstCapacity, compress_message, compress_size))){

			//fprintf(stderr, "1:decompress_size:%ld\n", decompress_size);
			write(decompress_fd, message, decompress_size);

		}
		//fprintf(stderr, "2:decompress_size:%lu\n", decompress_size);

	}
	gettimeofday(&end, NULL);
	decompress_time += time_elapsed(start, end);
	fprintf(stderr, "decompress time:%12.9f sec\n", decompress_time);
	close(decompress_fd);
	if(-1 == stat("/mnt/audit_compress_test/audit/audit.log.decompress", &statbuff)){
		fprintf(stderr, "stat error\n");
	}else{
		fprintf(stderr, "decompressed log size:%ld B\n",statbuff.st_size);
	}


	return 0;
}
