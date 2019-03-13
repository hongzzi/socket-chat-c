#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>

#define MAXLINE 4096
#define SA struct sockaddr
#define bzero(ptr,n)

int main(int argc, char **argv) {
	int sockfd, maxfdp1, n, pid;
	struct sockaddr_in servaddr;
	char line[MAXLINE], msg[MAXLINE+1], id[15], *esc = "/quit";
	fd_set rset;

	if(argc != 4) {
		printf("usage: ./ccli <IPaddress> <Port> <Id>\n");
		exit(1);
	}

	sprintf(id, "[%s]", argv[3]);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0) {
		printf("socket error\n");
		exit(1);
	}
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	servaddr.sin_port = htons(atoi(argv[2]) );

	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr) ) < 0) {
		printf("connect error\n");
		exit(1);
	}
	else {
		if (send(sockfd, id, strlen(id), 0) != strlen(id) ) {
			printf("send error\n");
			exit(1);
		}
	}

	maxfdp1 = sockfd + 1;
	FD_ZERO(&rset);
	
	while(1)	{
		FD_SET(0, &rset);
		FD_SET(sockfd, &rset);
		
		if (select(maxfdp1, &rset, NULL, NULL, NULL) < 0) {
			printf("select error\n");
			exit(1);
		}

		if (FD_ISSET(0, &rset) ) {
			if(fgets(msg, MAXLINE, stdin) )	{
				sprintf(line, "%s %s", id, msg);
				
				if (send(sockfd, line, strlen(line), 0) < 0) 					{
					printf("write error\n");
				}
				
				if (strstr(msg, esc) != NULL) {
					if (close(sockfd) == -1) {
						printf("close error\n");
						exit(1);
					}					
					exit(0);
				}
			}
		}

		if (FD_ISSET(sockfd, &rset) ) {
			int size;
			if ((size = recv(sockfd, msg, MAXLINE, 0) ) > 0) {
				msg[size] = '\0';
				printf("%s", msg);
				if (strstr(msg, esc) != NULL) {
					if (close(sockfd) == -1) {
						printf("close error\n");
						exit(1);
						}		
					printf("\n");			
					exit(0);
					}
				}
			}
		}
}

