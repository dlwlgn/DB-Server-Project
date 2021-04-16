# DBServerProject


## 구현 기능
- 서버와 클라이언트의 소켓 통신 구현
- 파일 입출력을 이용해 DB 관리
- 정해진 명령어(connect, save, read, clear, exit)로 클라이언트에서 서버에 저장된 DB 접근

## code

### server.c
```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

FILE *p_file = NULL;
char readbuf[100], temp[256], key[100], value[100], keyvalue[300], *find;
int fd, new_fd;

int init1()
{
	int ret = 0;
	fd = socket(AF_INET, SOCK_STREAM, 0);

	//1. socket
	if (fd == -1) {
		printf("socket error\n");
		return -1;
	}

	//2. binding
	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = 0;
	ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	if (ret == -1) {
		printf("bind error\n");
		return ret;
	}
}

int init2() {
	//3. listen state 
	int ret = 0;
	ret = listen(fd, 0);
	if (ret == -1) {
		printf("listen error\n");
		return ret;
	}
	printf("LISTEN...\n");
	return ret;
}

void bye()
{
	printf("\nbye\n");
	shutdown(new_fd, SHUT_WR);

	close(new_fd);
	close(fd);
	exit(0);
}
//클라이언트에서 넘어온 값을 처리
void rd() {
	while (1) {
		memset(readbuf, 0, 100);
		int rdcnt = read(new_fd, readbuf, sizeof(readbuf));
		//클라이언트가 종료될 때(길이 0을 보내옴)
		if (rdcnt == 0) {
			printf("client exit\n");
			return;
		}
		//클라이언트가 보내온 문자열을 파싱
		char *ptr = strtok(readbuf, "\n");
		char *ptr2 = strtok(ptr, " ");
    		//save 명령어가 왔을 때
		if (strcmp(ptr2, "save") == 0) {
			write(new_fd, "SAVE OK", 7);
			int flag = 0, line = 0, search = 0;
			ptr2 = strtok(NULL, ":");
			strcpy(key, ptr2);
			ptr2 = strtok(NULL, " ");
			strcpy(value, ptr2);
			//덮어쓸 문자열을 생성
			sprintf(keyvalue, "%s %s\n", key, value);
			//DB 텍스트 파일 열기
			p_file = fopen("db.txt", "r+t");
			//DB를 한줄씩 읽으며 
			while (fgets(temp, 256, p_file) != NULL) {
				find = strstr(temp, key);
				line++;
				if (find != NULL) {
					flag = 1;
					search = line;
				}
			}
			if (flag) {
				fseek(p_file, 0, SEEK_SET);
				line = 0;
				FILE *p_file2 = fopen("db_temp.txt", "w");
				while (fgets(temp, 256, p_file) != NULL) {
					line++;
					if (line == search) fputs(keyvalue, p_file2);
					else fputs(temp, p_file2);
				}
				fclose(p_file2);
				system("mv db_temp.txt db.txt");
			}
			else fputs(keyvalue, p_file);
			fclose(p_file);
		}
    		//read 명령어가 왔을 때
		else if (strcmp(ptr2, "read") == 0) {
			ptr2 = strtok(NULL, " ");
			strcpy(key, ptr2);
			p_file = fopen("db.txt", "r");
			while (fgets(temp, 256, p_file) != NULL) {
				find = strstr(temp, key);
				if (find != NULL) {
					ptr = strtok(temp, " ");
					ptr = strtok(NULL, "\n");
					write(new_fd, ptr, sizeof(ptr));
				}
			}
		}
    		//clear 명령어가 왔을 때
		else if (strcmp(ptr2, "clear") == 0) {
			write(new_fd, "CLEAR OK", 8);
			FILE *p_file2 = fopen("db_temp.txt", "w");
			fclose(p_file2);
			system("mv db_temp.txt db.txt");
		}
    		//exit 명령어가 왔을 때
		else if (strcmp(ptr2, "exit") == 0) {
			write(new_fd, "Client Shut Down", 16);
			shutdown(new_fd, SHUT_WR);
		}
    		//잘못된 명령어가 
		else write(new_fd, "Wrong Command", 13);

		sleep(1);
	}
}

int main()
{

	signal(SIGINT, bye);

	int ret = init1();
	if (ret == -1) {
		printf("INIT1 ERROR\n");
		return 0;
	}

	while (1) {
		ret = init2();
		if (ret == -1) {
			printf("INIT2 ERROR\n");
			return 0;
		}

		//accept (blocking)
		struct sockaddr new_addr = { 0 };
		int len;
		new_fd = accept(fd, &new_addr, &len);
		printf("START\n");

		//loop
		rd();

		close(new_fd);
	}

	close(fd);

	return 0;
}
```
### client.c
```c
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
		//connect 명령어를 받으면
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
```
