#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

int fd;
char readbuf[100];
char writebuf[100];

int init(char ip[]) 
{
	int ret = 0;

	//1. socket
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) return -1;

	//2. connect
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = inet_addr(ip);

	ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1) return ret;

	return ret;
}

void bye()
{
	printf("\nBYE\n");
	
	shutdown(fd, SHUT_WR);

	close(fd);
	exit(1);
}

void* wr(){
	while(1){
		printf("SHELL> ");
		fgets(writebuf, sizeof(writebuf), stdin);
		write(fd, writebuf, sizeof(writebuf));
		sleep(1);
	}
	return 0;
}

void* rd(){
	while(1){
		memset(readbuf, 0, 100);
		int rdcnt = read(fd, readbuf, sizeof(readbuf));
		if(rdcnt == 0) bye();
		printf("%s\n", readbuf);
		usleep(1000*100);
	}
	return 0;
}

int main()
{
	while(1){
		signal(SIGINT, bye);
		printf("SHELL> ");
		fgets(writebuf, sizeof(writebuf), stdin);
		char *p = strtok(writebuf, "\n");
		char *p2 = strtok(p, " ");
		
		if(strcmp(p2, "connect")==0){
			p2 = strtok(NULL, " ");
			int ret = init(p2);
			if (ret == -1) {
				printf("INIT ERROR\n");
				continue;
			}
		}
		else{
			printf("wrong command\n");
			continue;
		}

		
		pthread_t r, w;
		pthread_create(&r, NULL, rd, NULL);
		pthread_create(&w, NULL, wr, NULL);

		pthread_join(r, NULL);
		pthread_join(w, NULL);
	}
	return 0;
}
