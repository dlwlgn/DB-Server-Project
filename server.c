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
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = 0;
	ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	if (ret == -1) {
		printf("bind error\n");
		return ret;
	}
}

int init2(){
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

void rd(){
	while(1){
		memset(readbuf, 0, 100);
		int rdcnt = read(new_fd, readbuf, sizeof(readbuf));
		if(rdcnt == 0) {
			printf("client exit\n");
			return;
		}
	char *ptr = strtok(readbuf, "\n");
	char *ptr2 = strtok(ptr, " ");
	if(strcmp(ptr2,"save")==0){
		write(new_fd, "SAVE OK", 7);
		int flag = 0, line = 0, search = 0;
		ptr2 = strtok(NULL, ":");
		strcpy(key,ptr2);
		ptr2 = strtok(NULL, " ");
		strcpy(value,ptr2);
		sprintf(keyvalue,"%s %s\n",key,value);
		p_file = fopen("db.txt", "r+t");
		while(fgets(temp, 256, p_file) != NULL){
			find = strstr(temp, key);
			line++;
			if(find != NULL){
				flag = 1;
				search=line;	
			}
		}
		if(flag){
			fseek(p_file, 0, SEEK_SET);
			line = 0;
			FILE *p_file2 = fopen("db_temp.txt", "w");
			while(fgets(temp, 256, p_file) != NULL){
				line++;
				if(line == search) fputs(keyvalue, p_file2);
				else fputs(temp, p_file2);
			}
			fclose(p_file2);
			system("mv db_temp.txt db.txt");
		}
		else fputs(keyvalue,p_file);
		fclose(p_file);
	}
	else if(strcmp(ptr2, "read")==0){
		ptr2 = strtok(NULL, " ");
		strcpy(key,ptr2);
		p_file = fopen("db.txt", "r");
		while(fgets(temp, 256, p_file) != NULL){
			find = strstr(temp, key);
			if(find != NULL){
				ptr = strtok(temp, " ");
				ptr = strtok(NULL, "\n");
				write(new_fd, ptr, sizeof(ptr));
			}
		}
	}
	else if(strcmp(ptr2, "clear")==0){
		write(new_fd, "CLEAR OK", 8);
		FILE *p_file2 = fopen("db_temp.txt","w");
		fclose(p_file2);
		system("mv db_temp.txt db.txt");
	}
	else if(strcmp(ptr2, "exit")==0) {
		write(new_fd, "Client Shut Down",16);
		shutdown(new_fd, SHUT_WR);
	}
	else write(new_fd, "Wrong Command",13); 	

	sleep(1);
	}
}

int main()
{

	signal(SIGINT, bye);

	int ret = init1();
	if (ret == -1){
		printf("INIT1 ERROR\n");
		return 0;
	}
	
	while(1){
		ret = init2();
		if(ret == -1){
			printf("INIT2 ERROR\n");
			return 0;
		}

		//accept (blocking)
		struct sockaddr new_addr = {0};
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
