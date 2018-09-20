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
#include <sys/ioctl.h>
#include "../module/mailslot.h"

#define CMD_SET_MAX_MSG_SZ 0
#define CMD_GET_MAX_MSG_SZ 1
#define CMD_SET_MAX_MAILSLOT_SZ 2
#define CMD_GET_MAX_MAISLOT_SZ  3


int mailslot_open(char *path)
{
	int fd = open(path, O_RDWR);
	if(fd == -1){
		fprintf(stderr, "error opening file\n");
		exit(EXIT_FAILURE);
	}

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

int ms_ioctl(int fd, int request, int value)
{
	int ret;

	switch(request){
	case CMD_SET_MAX_MSG_SZ:
		request = MS_IOCSMSGSZ;
		break;
	case CMD_GET_MAX_MSG_SZ:
		request = MS_IOCGMSGSZ;
		break;
	case CMD_SET_MAX_MAILSLOT_SZ:
		request = MS_IOCSSZ;
		break;
	case CMD_GET_MAX_MAISLOT_SZ:
		request = MS_IOCGSZ;
		break;
	}

	ret = ioctl(fd, request, value);
	if(ret < 0) {
		fprintf(stderr, "request malformed. return error %d\n", ret);
		return ret;
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

int main(int argc, char *argv[])
{
	int request, value, fd, ret;
	char *path;

	path = argv[1];
	request = parse(argv[2]);
	if(argc > 3)
		value = parse(argv[3]);

	fd = mailslot_open(path);
	ret = ms_ioctl(fd, request, value);
	close(fd);

	fprintf(stdout, "ioctl return %d\n", ret);
	return ret;
}
