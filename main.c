#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h>

#include <stdbool.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>

#define _LARGEFILE64_SOURCE

#define FILENAME "in.dict"
#define BLOCK_SIZE 1024*1024*8

char* strrchr_fast (char* str_start, char* reverse_start_pos, char c)
{
	for(char* pointer = reverse_start_pos; pointer >= str_start; pointer--)
	{
		if (c != '\0' && *pointer == '\0')
		{
			printf("strrchr_fast(): unexpected null-character!\n");
			exit(-1);
		}
		if(*pointer == c)
			return pointer;
	}
	return NULL;
}

void checkBlockSize()
{
	if(BLOCK_SIZE > SSIZE_MAX)
	{
		printf("PLEASE RECOMPILE WITH ANOTHER BLOCK_SIZE\n");
		exit(-1);
	}
}

int64_t getFileSize(char* filename)
{
	int64_t size = -1;
	if(filename)
	{
		int fd = open(FILENAME, O_RDONLY);
		if(fd != -1)
		{
			size = lseek64(fd, 0, SEEK_END);
			close(fd);
		}
		else
		{
			printf("getFileSize(): open error\n");
			exit(-1);
		}
	}
	return size;
}



ssize_t readBlock(int fd, char* buffer)
{
	if(fd == -1 || buffer == NULL)
	{
		printf("ERROR: readBlock(int fd = 0x%08x, char* buffer = %p)\n", fd, buffer);
		exit(-1);
	}
	ssize_t readed = read(fd, buffer, BLOCK_SIZE-1);
	if (readed > 0)
	{
		char* last_new_line = strrchr_fast(buffer, buffer + readed - 1, '\n');
		*last_new_line = '\0';
		ssize_t real_readed = last_new_line - buffer;
		lseek64(fd, real_readed - readed, SEEK_CUR);
		return real_readed;
	}
	return 0;
}

int processingBlock(char* block, ssize_t length, char* charset_out, int* charset_counter_out)
{
	int count_lines_in_block = 1;
	bool first_symbol = true;
	int carrage_ret_cnt = 0;

	for(int i = 0; i < length; i++)
	{
		if (block[i] == '\0')
		{
			first_symbol = true;
			continue;
		}

		if(block[i] == '\r')
			if (i != length - 1 && block[i + 1] == '\n')
			{
				block[i+1] = block[i] = '\0';
				carrage_ret_cnt++;
			}
			else
			{
				block[i] = '\0';
			}
		
		if (block[i] == '\n')
		{
			block[i] = '\0';
			count_lines_in_block++;
			first_symbol = true;
		}

		if (first_symbol)
		{
			char* pos = strchr(charset_out, block[i]);
			if (!pos)
			{
				charset_out[strlen(charset_out)] = block[i];
				charset_counter_out[strlen(charset_out)] = 1;
			}
			else
			{
				charset_counter_out[pos - charset_out]++;
			}
		}
		if (block[i] == '\n')
		{
			block[i] = '\0';
			count_lines_in_block++;
			first_symbol = true;
		}
	}

	printf("carrage_ret_cnt = %d\n", carrage_ret_cnt);
	return count_lines_in_block;
}

int main()
{
	checkBlockSize();
	
	int64_t size = getFileSize(FILENAME);
	if(size == 0)
	{
		printf("int main(): empty file\n");
		exit(-1);
	}

	printf("FileSize = %ld\n", size);
	char* buffer = malloc(BLOCK_SIZE);
	int dict_fd = open(FILENAME, O_RDONLY);
	ssize_t bytes_readed;
	int count_lines_all = 1;
	bool first_run = true;
	
	int charset_population = 0;
	char charset[256];
	unsigned int charset_counter[256];

	memset(charset, '\0', 256 * sizeof(char));
	memset(charset_counter, '\0', 256 * sizeof(unsigned int));

	while(bytes_readed = readBlock(dict_fd, buffer))
	{
		count_lines_all += processingBlock(buffer, bytes_readed, charset, charset_counter);
	}

	printf("Lines in file: %d\n", count_lines_all);
	
	printf("Charset: \"%s\"\n", charset);
	
	lseek64(dict_fd, 0L, SEEK_SET);
	char** pointers = malloc(count_lines_all * sizeof(char*));
	if(!pointers)
	{
		printf("int main(): malloc(count_lines * sizeof(char*)) failed!\n");
		exit(-1);
	}

	int64_t counter = 0;
	// while(bytes_readed = readBlock(dict_fd, buffer))
	// {	
	// 	char* iterator = buffer;
	// 	//printf("bytes_readed = %ld\n", bytes_readed);
	// 	while(iterator - buffer < bytes_readed)
	// 	{	
	// 		printf("buffer = %p; iterator = %p; difference = %ld; bytes_readed = %ld\n", buffer, iterator, iterator-buffer, bytes_readed);
	// 		pointers[counter++] = iterator;
	// 		printf("strlen(iterator) = %lu\n", strlen(iterator));
	// 		iterator += strlen(iterator) + 1;

	// 	}
	// }

	printf("Created %ld pointers!\n", --counter);

	close(dict_fd);
}