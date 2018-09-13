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

void mailslot_read(int fd, int size)
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
		return;
	} else if(r==0){
		fprintf(stderr, "no msg to read: maybe you passed a buffer too short\n");
		return;
	}

	printf("%s\n",buf);
}

void mailslot_write(int fd, char *data)
{
	int r;

	errno = 0;
	r = write(fd, data, strlen(data));
	if(r == -1)
		fprintf(stderr, "error %d\n", errno);

	printf("write succeed: %s len:%d\n", data, (int) strlen(data));
}

int doesFileExist(const char *filename)
{
	struct stat st;
	int result = stat(filename, &st);
	return result == 0;
}

int main(int argc, char **argv)
{
	char *basepath = "/dev/";
	char *name, *cmd;
	char path[MAX_LINE];
	int n, fd, len, size, ret;
	unsigned int major;
	unsigned int minor;
	dev_t dev;
	pid_t pid;

	if(argc < 6){
		fprintf(stderr, "Usage: <prog> <name> <major> <minor> <read|write> <size|msg> \n");
		exit(EXIT_FAILURE);
	}

	name = argv[1];
	errno = 0;
	major = strtol(argv[2], NULL, 0);
	if(errno != 0){
		fprintf(stderr, "2th argument must be a valid int\n");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	minor = strtol(argv[2],NULL,0);
	if(errno != 0){
		fprintf(stderr, "3th argument must be a valid int\n");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	minor = strtol(argv[3],NULL,0);
	if(errno != 0){
		fprintf(stderr, "4th argument must be a valid int\n");
		exit(EXIT_FAILURE);
	}

	cmd = argv[4];
	strcpy(path, basepath);
	strcat(path, name);

	if(!doesFileExist(path)){
		dev = makedev(major, minor);
		errno = 0;
		ret = mknod(path, 666 | S_IFCHR,  dev);
		if(ret == -1){
			fprintf(stderr, "error creating device node at %s\n", path);
		exit(EXIT_FAILURE);
		}
	}

	fd = open(path, O_RDWR);
	if(fd == -1){
		fprintf(stderr, "device not exist\n");
		exit(EXIT_FAILURE);
	}

	if(!strcmp(cmd, "read")){
		errno = 0;
		size = strtol(argv[5], NULL, 0);
		if( errno != 0){
			fprintf(stderr, "6th argument must be a valid int\n");
			exit(EXIT_FAILURE);
		}

		mailslot_read(fd, size);

	} else if(!strcmp(cmd, "write")){
		mailslot_write(fd, argv[5]);
	}

	close(fd);

	exit(EXIT_SUCCESS);
}
