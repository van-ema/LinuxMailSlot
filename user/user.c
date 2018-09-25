/*
 *   Copyright (C) 2018 Emanuele Vannacci
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sysmacros.h>

#define MAX_LINE 1024
#define BLOCKING 0
#define NO_BLOCKING 1

int mailslot_open(char *path, int policy)
{
	int flags = policy == BLOCKING ? O_RDWR : O_RDWR | O_NONBLOCK;
	int fd = open(path, flags);
	if(fd == -1){
		fprintf(stderr, "error opening file\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "file opened, policy:%d (0 for BLOCKING)\n", policy);
	return fd;
}
int mailslot_close(int fd)
{
	int ret = close(fd);
	if(ret !=0){
		fprintf(stderr, "error closing file\n");
		exit(EXIT_FAILURE);
	}

	return ret;
}

int mailslot_read(int fd, int size)
{
	int r;
	char *buf;

	buf = malloc(size);
	if(buf==NULL){
		fprintf(stderr, "memory allocation error\n");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	r = read(fd, buf, size);
	if(r==-1 || errno != 0){
		fprintf(stderr,"error %d\n", errno);
		exit(EXIT_FAILURE);
	} else if(r==0){
		fprintf(stderr, "no msg to read: maybe you passed a buffer too short\n");
		exit(EXIT_FAILURE);
	}

	printf("%s\n",buf);
	return r;
}

int mailslot_write(int fd, char *data)
{
	int r;

	errno = 0;
	r = write(fd, data, strlen(data));
	if(r == -1){
		fprintf(stderr, "error %d\n", errno);
		exit(EXIT_FAILURE);
	}
	printf("write succeed: %s len:%d\n", data, (int) strlen(data));
	return r;
}

int doesFileExist(const char *filename)
{
	struct stat st;
	int result = stat(filename, &st);
	return result == 0;
}

int mailslot_create(char *path, int major, int minor)
{
	dev_t dev;
	int ret;

	if(doesFileExist(path)){
		fprintf(stderr, "file already exist\n");
		return -1;
	}

	dev = makedev(major, minor);
	errno = 0;
	ret = mknod(path, 0666 | S_IFCHR,  dev);
	if(ret == -1){
		fprintf(stderr, "error creating device node at %s\n", path);
		exit(EXIT_FAILURE);
	}

	return ret;
}

int parse(char *arg)
{
	int val;
	errno = 0;
	val = strtol(arg, NULL, 0);
	if(errno != 0){
		fprintf(stderr, "2th argument must be a valid int\n");
		exit(EXIT_FAILURE);
	}

	return val;
}

int main(int argc, char **argv)
{
	char *basepath = "/dev/";
	char *name, *cmd;
	char path[MAX_LINE];
	int fd,size;
	int block = 1;
	unsigned int major;
	unsigned int minor;

	if(argc < 4){
		fprintf(stderr, "Usage: <prog> <cmd> <filename> [args] \n");
		exit(EXIT_FAILURE);
	}


	cmd = argv[1];
	name = argv[2];
	strcpy(path, basepath);
	strcat(path, name);

	if(strcmp(cmd, "create") == 0) {
		major = parse(argv[3]);
		minor = parse(argv[4]);
		mailslot_create(path, major, minor);
		exit(EXIT_SUCCESS);
	} else if (strcmp(cmd, "read") == 0) {
		size = parse(argv[3]);
		if(argc > 4) block = parse(argv[4]);
		fd = mailslot_open(path, block);
		mailslot_read(fd, size);
	} else if(strcmp(cmd, "write") == 0) {
		if(argc > 4) block = parse(argv[4]);
		fd = mailslot_open(path, block);
		mailslot_write(fd, argv[3]);
	} else {
		fprintf(stdout, "Usage: <prog> <cmd> <filename> [args] \n");
		exit(EXIT_SUCCESS);
	}

	close(fd);

	exit(EXIT_SUCCESS);
}
