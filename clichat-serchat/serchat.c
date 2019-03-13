#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAXLINE 4096
#define MAXSOCK 256
#define SA struct sockaddr

int maxfdp1, idnum = 0;
int clisock[MAXSOCK];
char id[MAXSOCK][10], line[MAXLINE], *esc = "/quit";

int getmax(int k) {
	int max = k;   
	int l;

	for (l = 0; l < idnum; l++) {
		if (clisock[l] > max ) max = clisock[l];
	}
	return max;
}

void cliclose(int i)
{
	int j;
	char msg[MAXLINE];

	sprintf(msg, "%s leaved.\n", id[i]);
	
	close(clisock[i]);

	if(i != idnum-1)
	{
		clisock[i] = clisock[idnum-1];
		sprintf(id[i], "%s", id[idnum-1]);
		id[idnum-1][0] = '\0';
  	}
	idnum--;

	for(j=0; j<idnum; j++)
		send(clisock[j], msg, strlen(msg), 0);
}

void serclose(int i)
{
	char *msg = "Server is closed";
	int j;
	for(j = 0; j < idnum; j++) {
		if (send(clisock[j], msg, strlen(msg), 0) != strlen(msg) ) {
			printf("send error\n");
			exit(1);
		}
		if (close(clisock[j]) == -1) {
			printf("close error\n");
			exit(1);
		}	
	}
	if (close(i) == -1) {
		printf("close error\n");
		exit(1);
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	char mymsg[MAXLINE], msg[MAXLINE], *ptr, buff[MAXLINE];
	int i, j, n;
	int listenfd, connfd, clilen, backlog;
	socklen_t len;
	fd_set rset;
	struct sockaddr_in cliaddr, servaddr, localaddr, peeraddr;
	len = sizeof(localaddr);

	if(argc != 2) {
		printf("usage: ./ccser <Port>\n", argv[1]);
		exit(1);
	}
 
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) {
			printf("socket error\n");
			exit(1);
	}

	bzero(&servaddr, sizeof(servaddr) );
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]) );

	if ( bind(listenfd, (SA *) &servaddr, sizeof(servaddr) ) < 0 ) {
			printf("bind error\n");
			exit(1);
	}

	if ((ptr = getenv("LISTENQ") ) != NULL)
			backlog = atoi(ptr);

	if (listen(listenfd, backlog) < 0 ) {
			printf("listen error\n");
			exit(1);
	}
	
	if ( getsockname(listenfd, (SA *) &localaddr, &len) < 0) {
		printf("error\n");
		exit(1);
	}
	printf("[server address is %s : %d]\n",
	inet_ntop(AF_INET, &localaddr.sin_addr, buff, sizeof(buff) ), ntohs(localaddr.sin_port) );

	maxfdp1 = listenfd + 1;

	while(1) {
		msg[0] = '\0';
    	line[0] = '\0';
    	FD_ZERO(&rset);
		FD_SET(listenfd, &rset);
		FD_SET(0, &rset);

		for(i = 0; i < idnum; i++)
			FD_SET(clisock[i], &rset);

    	maxfdp1 = getmax(listenfd) + 1;

		if ((select(maxfdp1, &rset, (fd_set *)0, (fd_set *)0, (struct timeval *)0) ) < 0) {
			printf("select error\n");
			exit(1);
		}
		
		if(FD_ISSET(listenfd, &rset) ) {
			clilen = sizeof(cliaddr);
		again:
			if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen) ) < 0) {
		#ifdef	EPROTO
				if (errno == EPROTO || errno == ECONNABORTED)
		#else
				if (errno == ECONNABORTED)
		#endif
					goto again;
				else {
					printf("accept error\n");
					exit(1);
				}
			}

			clisock[idnum] = connfd;
			n = recv(connfd, line, MAXLINE, 0);
		   line[n] = '\0';
  	   	sprintf(id[idnum], "%s", line);

      	getsockname(connfd, (SA *)&cliaddr, &clilen);

      	printf("%s is connected from %s\n", id[idnum], inet_ntoa(cliaddr.sin_addr) );
             
        	sprintf(msg, "[%s is connected]\n", id[idnum]);
      	idnum++;
        	for(j=0; j < idnum-1; j++) {
         	send(clisock[j], msg, strlen(msg), 0);
        		}
       		}

		if(FD_ISSET(0, &rset) ) {
			if(fgets(mymsg, MAXLINE, stdin) ) {
				if (strstr(mymsg, esc) != NULL)
         		serclose(listenfd);     

				sprintf(msg, "[admin] %s", mymsg);

				for(i = 0; i < idnum; i++) {
					if(send(clisock[i], msg, strlen(msg), 0) < 0)
          			printf("set error to ...\n");
				}
			}
		}
	
		for(i = 0; i < idnum; i++)	{
      	if(FD_ISSET(clisock[i], &rset) ) {
        		if((n = recv(clisock[i], line, MAXLINE, 0) )  <= 0) {
          		cliclose(i);
          		continue;
       				 }

        		line[n] = '\0';
				
				getpeername(connfd, (SA *)&peeraddr, &clilen);

        		if(strstr(line, esc) != NULL) {
          		cliclose(i);
					printf("%s leaved at %s\n", id[i],
					inet_ntop(AF_INET, &peeraddr.sin_addr, buff, sizeof(buff) ) );
          		continue;
			}

           	for (j = 0; j < idnum; j++) {
           		if (i != j) {
           	   	send(clisock[j], line, n, 0);
					}
       				}

        		printf("%s", line);
			}
		}
 	 }
}
