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

int audit_fd = -1;
int audit_fd_origin = -1;
int audit_text_fd = -1;
static struct timeval start, end;
static struct timeval start_storage, end_storage;
#define SIZE_T_MAX 20
#define ChunkSize 1*1024*1024
//#define MAX_COMPRESS MAX_AUDIT_MESSAGE_LENGTH
//#define MAX_COMPRESS ChunkSize 
#define dstCapacity LZ4_COMPRESSBOUND(ChunkSize)

static float time_elapsed(struct timeval _start, struct timeval _end)
{
	return (float)(_end.tv_sec - _start.tv_sec) + (float)(_end.tv_usec - _start.tv_usec)/1000000.0f;
}

int main(int argc, char **argv)
{
	size_t msg_len = 0;
	char message[MAX_AUDIT_MESSAGE_LENGTH] = { 0 };
	audit_fd = open("audit.log.compress",O_CREAT|O_RDWR,0666);
	//audit_fd_origin = open("/mnt/audit_compress_test/compress/audit.log.origin",O_CREAT|O_RDWR,0666);
	//audit_text_fd = open("/mnt/audit_compress_test/audit.log.example_11.18",O_CREAT|O_RDWR,0666);

	char msg_buffer[SIZE_T_MAX+1] = { 0 };
	size_t n = 0;
	char compress_message[dstCapacity] = { 0 };
	size_t compress_size = 0;
	size_t accumulated_size = 0;
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


		while((n=read(0, msg_buffer, SIZE_T_MAX+1))){
		//while((msg_len=read(0, message, MAX_AUDIT_MESSAGE_LENGTH))){	

			msg_len = atol(msg_buffer);
            		
			//for testing the write()
			//write(audit_fd, msg_buffer, SIZE_T_MAX+1);
			
			gettimeofday(&start, NULL);
			//fprintf(stderr, "message:%s\n",message);
			if (msg_len<=0)
				fprintf(stderr, "msg_len:%ld\n",msg_len);
			if (temp_flag){
				strncpy(message_chunk, temp_message, offset);
				temp_flag=0;
				//fprintf(stderr, "message_chunk:%s\n", message_chunk);
				//fprintf(stderr, "exit temp\n");
			}
			read(0, message, msg_len);
			//fprintf(stderr, "n:%ld\n", n);
			//fprintf(stderr, "message[%ld]:%d\n",msg_len-1, message[msg_len-1]);
			
			//write -> compress function
            		//write(audit_fd_origin, message, msg_len);
			//write(audit_fd, message, MAX_AUDIT_MESSAGE_LENGTH);

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
				if((compress_size = LZ4_compress_default(message_chunk, compress_message, accumulated_size, LZ4_compressBound(accumulated_size)))){
					//if(0==compress_size)
					//	fprintf(stderr, "error in compress\n");
					snprintf(compressed_size, SIZE_T_MAX+1,"%lu", compress_size);
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
                     	//fprintf(stderr,"accumuated_size:%ld LZ4_MAX_INPUT_SIZE:%d\n", accumulated_size, LZ4_MAX_INPUT_SIZE);
			/*
			//if (accumulated_size >= MAX_COMPRESS){
				if((compress_size = LZ4_compress_default(message, compress_message, accumulated_size, LZ4_compressBound(accumulated_size)))){
					//if(0==compress_size)
					//	fprintf(stderr, "error in compress\n");
					snprintf(compressed_size, SIZE_T_MAX+1,"%lu", compress_size);
					//fprintf(stderr, "%lu\n", compress_size);
					gettimeofday(&start_storage, NULL);
					write(audit_fd, compressed_size, SIZE_T_MAX+1);
					write(audit_fd, compress_message, compress_size);
					gettimeofday(&end_storage, NULL);
					storage_time += time_elapsed(start_storage, end_storage);	
				}
				accumulated_size = 0;
			//}
			*/
			gettimeofday(&end, NULL);
			compress_time += time_elapsed(start, end);
			//fprintf(stderr, "end one if\n");

		}

		//compress the leave contents before the program ends
		if (temp_flag){
			strncpy(message_chunk, temp_message, offset);
			temp_flag=0;
			//fprintf(stderr, "message_chunk:%s\n", message_chunk);
			//fprintf(stderr, "exit temp\n");
		}

		gettimeofday(&start, NULL);
		if((compress_size = LZ4_compress_default(message_chunk, compress_message, accumulated_size, LZ4_compressBound(accumulated_size)))){
					//if(0==compress_size)
					//	fprintf(stderr, "error in compress\n");
					snprintf(compressed_size, SIZE_T_MAX+1,"%lu", compress_size);
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
		fprintf(stderr, "Accumulated storage time:%12.9f sec\n", storage_time);
		fprintf(stderr, "Accumulated compress time:%12.9f sec\n", compress_time-storage_time);

		if(-1 == stat("audit.log.origin", &statbuff)){
			fprintf(stderr, "stat error\n");
		}else{
			fprintf(stderr, "original log size:%ld B\n",statbuff.st_size);
		}
		if(-1 == stat("audit.log.compress", &statbuff)){
			fprintf(stderr, "stat error\n");
		}else{
			fprintf(stderr, "compressed log size:%ld B\n",statbuff.st_size);
		}

		close(audit_fd);

		//validate the function of decompression
		audit_fd = open("audit.log.compress",O_CREAT|O_RDWR,0666);
		int decompress_fd = open("audit.log.decompress",O_CREAT|O_RDWR,0666);
		while((n=(read(audit_fd, compressed_size, SIZE_T_MAX+1)))){
			//fprintf(stderr, "n:%ld\n", n);
			compress_size = atol(compressed_size);
			//fprintf(stderr, "compress_size:%ld\n", compress_size);
			read(audit_fd, compress_message, compress_size);
			if((decompress_size = LZ4_decompress_safe(compress_message, message, compress_size, dstCapacity))){
				//fprintf(stderr, "1:decompress_size:%ld\n", decompress_size);
				write(decompress_fd, message, decompress_size);

			}
			//fprintf(stderr, "2:decompress_size:%lu\n", decompress_size);

		}
		close(decompress_fd);
		if(-1 == stat("audit.log.decompress", &statbuff)){
			fprintf(stderr, "stat error\n");
		}else{
			fprintf(stderr, "decompressed log size:%ld B\n",statbuff.st_size);
		}

		



	return 0;
}
