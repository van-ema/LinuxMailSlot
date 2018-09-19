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
#include <pthread.h>

#define N 5
#define OP 1000000
#define MSG_SZ 16

pthread_t tid[N];


int open_file(char *path){

	int fd = open(path, O_RDWR);
	if(fd == -1){
		fprintf(stderr, "cannot open %s\n", path);
		exit(EXIT_FAILURE);
	}

	return fd;
}


void *execute_read(void *arg){
	int i, *fd, ret;
	char *msg;

	fd = (int *) arg;
	msg = malloc(sizeof(char) * MSG_SZ);
	if(msg == NULL){
		fprintf(stderr, "memory allocation error\n");
		exit(EXIT_FAILURE);
	}

	for(i = 0; i <OP; i++){
		ret = read(*fd,msg, MSG_SZ);
		if(ret == -1){
			fprintf(stderr, "write error\n");
			exit(EXIT_FAILURE);
		}

	}
}

void *execute_write(void *arg){
	int i,*fd, ret;
	char *msg;

	fd = (int *) arg;
	fprintf(stderr, "file descriptor %d\n", *fd);

	msg = malloc(sizeof(char) * MSG_SZ);
	if(msg == NULL){
		fprintf(stderr, "memory allocation error\n");
		exit(EXIT_FAILURE);
	}

	msg = memset(msg, 'a', MSG_SZ);

	for(i = 0; i <OP; i++){
		int missing = MSG_SZ;
		errno = 0;
		while( (ret = write(*fd, msg, missing)) != 0){
			if(ret == -1){
				fprintf(stderr, "write error %d\n", errno);
				exit(EXIT_FAILURE);
			}
			missing -= ret;
		}
	}

}

void spawn_thread( void *(*routine) (void *), int fd){
	int i, ret;
	for( i=0; i<N; i++){
		ret = pthread_create(&tid[i], NULL, routine, (void *) &fd);
		if (ret != 0){
			fprintf(stderr, "error in creating thread\n");
			exit(EXIT_FAILURE);
		}
	}

}

int main(int argc, char *argv[])
{
	int fd, i;

	if(argc < 3){
		fprintf(stdout, "Usage: <prog> <path> <read|write>\n");
		exit(EXIT_FAILURE);
	}

	fd = open_file(argv[1]);
	if(strcmp(argv[2], "read")==0){
		spawn_thread(execute_read, fd);
	} else if( strcmp(argv[2], "write") == 0) {
		spawn_thread(execute_write, fd);
	}

	for(i=0; i<N; i++)
		pthread_join(tid[i], NULL);

	close(fd);
	exit(EXIT_SUCCESS);

}
