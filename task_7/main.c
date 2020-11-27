#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include "vector.h"

#define TIMEOUT ((int)5000)
#define FILE_TIMEOUT_MS ((int)5000)

int wait_for_input(int file_des, int timeout_ms)
{
	struct pollfd poll_file_des = {
		.fd = file_des,
		.events = POLLIN,
		.revents = 0
	};
	return poll(&poll_file_des, 1, timeout_ms);
}

bool fill_table(vector_t* table, int file_des)
{
	struct stat file_stat = {0};

	if (fstat(file_des, &file_stat) != 0)
	{
		perror("Error while filling the table: ");
		return true;
	}
	int old_flags = fcntl(file_des, F_GETFL); // Set file_des in nonblock mode
	fcntl(file_des, F_SETFL, old_flags | O_NONBLOCK);
	char* file_buffer = NULL;
	errno = 0;

	if ( (file_buffer = mmap((caddr_t) 0, file_stat.st_size, PROT_READ,
							MAP_SHARED, file_des, 0)) == MAP_FAILED )
	{
		// If mmap(...) set errno in EAGAIN, trying to read data from file
		// once again with timeout for TIMEOUT ms.
		//
		if (errno == EAGAIN)
		{
			int wait_res = wait_for_input(file_des, FILE_TIMEOUT_MS);

			if (wait_res == 0)
               		 {
                    		fcntl(file_des, F_SETFL, old_flags);
                   		perror("Timeout error, could not open file: ");
                    		return true;
                	}
			else if (wait_res == -1)
			{
				perror("Error while waiting from stdin:  ");
				return true;
			}
		}
	}
	char* n_pos = file_buffer;

	while ((n_pos = strchr(n_pos, '\n')) != NULL)
	{
		if (vector_push_back(table, n_pos - file_buffer))
		{
			perror("fill_table error, cannot to add element to array: ");
			fcntl(file_des, F_SETFL, old_flags);
			return true;
		}
		++n_pos;
	}
    //Return old mode for file_des
    //
    fcntl(file_des, F_SETFL, old_flags);
    // Unmap mapped file
	//
	if (munmap(file_buffer, file_stat.st_size) == -1)
		perror("Error while unmapping file from memory: ");

	return false;
}

void print_line(int file_des, vector_t* table, size_t line_number)
{
    off_t line_pos = line_number == 0
            ? 0
            : vector_get(table, line_number - 1) + 1;

    size_t line_length = vector_get(table, line_number) - line_pos + 1;
    char* line = calloc(line_length + 1, sizeof(char));

    if (line == NULL)
    {
        perror("Print line error: ");
        return;
    }
    lseek(file_des, line_pos, SEEK_SET);
    read(file_des, line, line_length);
    printf("%s", line);
    free(line);
}

bool line_manage(int file_des, vector_t* table, size_t line_num)
{
	if (line_num == 0)
		return true;

	else if (line_num <= table->size)
	{
		printf("%lu|", line_num);
		print_line(file_des, table, line_num - 1);
	}
	else
		printf("Incorrect line number. Try again:\n");

	return false;
}

void scan_luint(size_t* value)
{
	while(scanf("%lu", value) == 0)
	{
		printf("Incorrect input. Try to input integer value:\n");
		fflush(stdin);
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Invalid argument\nUsage: prog [filename]\n");
		return EXIT_FAILURE;
	}
	int file_des = open(argv[1], O_RDONLY);

	if (file_des == -1)
	{
		perror("Cannot open input file\n");
		return EXIT_FAILURE;
	}
	vector_t table;
	size_t line_num = 0;

	if (vector_init(&table) || fill_table(&table, file_des))
	{
		perror("Error, cannot create table\n");
		vector_destroy(&table);
		return EXIT_FAILURE;
	}
	printf(
			"Input line number in file.\n"
			"After %d seconds program will print all file and close.\n"
			"Input 0 to exit:\n",
			TIMEOUT / 1000
	);
	for(int result = 0;; result =  wait_for_input(0 /*stdin*/, TIMEOUT;)
	{
		if (result == 1)
		{
			scan_luint(&line_num);

			if (line_manage(file_des, &table, line_num))
				break;
		}
		else if (result == 0)
		{
			printf("Input timeout\n");

			for (size_t i = 1; i <= table.size; ++i)
			{
				line_manage(file_des, &table, i);
			}
			break;
		}
		else (result == -1)
		{
			perror("Error while wait for input from stdin: ");
			break;
		}
	}
	close (file_des);
	vector_destroy(&table);
	return EXIT_SUCCESS;
}
