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

int mailslot_create(char *path, int major, int minor)
{
	dev_t dev;
	int ret;

	dev = makedev(major, minor);
	errno = 0;
	ret = mknod(path, 666 | S_IFCHR,  dev);
	if(ret == -1){
		fprintf(stderr, "error creating device node at %s\n", path);
		exit(EXIT_FAILURE);
	}

	return ret;
}

int fifo_create(char *path){
	int ret;

	ret = mkfifo(path, 0666 | O_CREAT);
	if(ret == -1){
		fprintf(stderr, "error creating fifo\n");
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

int main( int argc, char *argv[])
{
	char *cmd, *path;
	if(argc < 3){
		fprintf(stdout,"Usage: <prog> <fifo|mailslot> <path> [<major> <minor>]\n");
		exit(EXIT_FAILURE);
	}

	cmd = argv[1];
	path = argv[2];

	if(strcmp(cmd, "fifo") ==0 ) {
		fprintf(stdout, "creating fifo...\n");
		fifo_create(path);
	} else if(strcmp(cmd, "mailslot") == 0){
		fprintf(stdout, "creating mainslot...\n");
		int major = parse(argv[3]);
		int minor = parse(argv[4]);
		mailslot_create(path, major, minor);
	}

	exit(EXIT_SUCCESS);
}
