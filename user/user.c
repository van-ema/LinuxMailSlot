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
#define THRESHOLD 0.5

int mailslot_open(char *path)
{
	int fd = open(path, O_RDWR);
	if(fd == -1){
		fprintf(stderr, "error opening file\n");
		exit(EXIT_FAILURE);
	}

	return fd;
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
	ret = mknod(path, 666 | S_IFCHR,  dev);
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
	int fd, ret, size;
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
		ret = mailslot_create(path, major, minor);
	} else if (strcmp(cmd, "read") == 0) {
		size = parse(argv[3]);
		fd = mailslot_open(path);
		ret = mailslot_read(fd, size);
	} else if(strcmp(cmd, "write") == 0) {
		fd = mailslot_open(path);
		ret = mailslot_write(fd, argv[3]);
	} else {
		fprintf(stdout, "Usage: <prog> <cmd> <filename> [args] \n");
		exit(EXIT_SUCCESS);
	}

	exit(EXIT_SUCCESS);
}
