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
//#include <zstd.h>
//#include <lz4.h>
//#include <lz4hc.h>
#include <zstd.h>

int audit_fd = -1;
static struct timeval start, end;
#define SIZE_T_MAX 20
#define MAX_COMPRESS MAX_AUDIT_MESSAGE_LENGTH
#define dstCapacity ZSTD_COMPRESSBOUND(MAX_COMPRESS)

static float time_elapsed(struct timeval _start, struct timeval _end)
{
	return (float)(_end.tv_sec - _start.tv_sec) + (float)(_end.tv_usec - _start.tv_usec)/1000000.0f;
}


int main(int argc, char **argv)
{
	size_t msg_len = 0;
	char message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	audit_fd = open("/mnt/audit_compress_test/compress/audit.log.compress",O_CREAT|O_RDWR,0666);
	char msg_buffer[SIZE_T_MAX+1] = { 0 };
	size_t n = 0;
	char compress_message[dstCapacity] = { 0 };
	int compress_size = 0;
	int accumulated_size = 0;
	float compress_time = 0;
	struct stat statbuff;
	size_t decompress_size = 0;
	char compressed_size[SIZE_T_MAX+1] = { 0 };

		while((n=read(0, msg_buffer, SIZE_T_MAX+1))){	
			msg_len = atol(msg_buffer);
            		
			//for testing the write()
			//write(audit_fd, msg_buffer, SIZE_T_MAX+1);
			
			gettimeofday(&start, NULL);
			
			read(0, message, msg_len);
			
			//write -> compress function
            		//write(audit_fd, message, msg_len);

			accumulated_size += msg_len;
                     	//fprintf(stderr,"accumuated_size:%ld LZ4_MAX_INPUT_SIZE:%d\n", accumulated_size, LZ4_MAX_INPUT_SIZE);
			//if (accumulated_size >= MAX_COMPRESS){
				if((compress_size = ZSTD_compress(compress_message, ZSTD_compressBound(accumulated_size), message, accumulated_size, 1))){
					snprintf(compressed_size, SIZE_T_MAX+1,"%d", compress_size);
					//fprintf(stderr, "%lu\n", compress_size);
					write(audit_fd, compressed_size, SIZE_T_MAX+1);
					write(audit_fd, compress_message, compress_size);
				}
				accumulated_size = 0;
			//}
			gettimeofday(&end, NULL);
			compress_time += time_elapsed(start, end);
		
		}

		/*gettimeofday(&start, NULL);
		if((compress_size = ZSTD_compress(compress_message, ZSTD_compressBound(accumulated_size), message, accumulated_size, 1))){
			write(audit_fd, compress_message, compress_size);
		}	
		gettimeofday(&end, NULL);                 	
                compress_time += time_elapsed(start, end);*/
		fprintf(stderr, "Accumulated compress time:%12.9f sec\n", compress_time);

		if(-1 == stat("/mnt/audit_compress_test/compress/audit.log.origin", &statbuff)){
			fprintf(stderr, "stat error\n");
		}else{
			fprintf(stderr, "original log size:%ld B\n",statbuff.st_size);
		}
		if(-1 == stat("/mnt/audit_compress_test/compress/audit.log.compress", &statbuff)){
			fprintf(stderr, "stat error\n");
		}else{
			fprintf(stderr, "compressed log size:%ld B\n",statbuff.st_size);
		}

		close(audit_fd);

		//validate the function of decompression
		audit_fd = open("/mnt/audit_compress_test/compress/audit.log.compress",O_CREAT|O_RDWR,0666);
		int decompress_fd = open("/mnt/audit_compress_test/compress/audit.log.decompress",O_CREAT|O_RDWR,0666);
		while((n=(read(audit_fd, compressed_size, SIZE_T_MAX+1)))){
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
		close(decompress_fd);
		if(-1 == stat("/mnt/audit_compress_test/compress/audit.log.decompress", &statbuff)){
			fprintf(stderr, "stat error\n");
		}else{
			fprintf(stderr, "decompressed log size:%ld B\n",statbuff.st_size);
		}


	return 0;
}
