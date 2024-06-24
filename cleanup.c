#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_MSG_SIZE 256

struct msgbuf {
    long mtype;
    char mtext[MAX_MSG_SIZE];
};

int main() {
   
    key_t key = ftok("client.c", 'C');
    	if(key==-1){
	perror("error in ftok()");
        exit(1);
	}
    
    int msgid = msgget(key, 0666 );

    if(msgid == -1) {
        perror("error in msgget()");
        exit(1);
    }
     	struct msgbuf message;
        message.mtype = 99;
	
	while(1){
		printf("Do you want the server to terminate? Press 1 for Yes and 0 for No.\n");
		int choice;
		scanf("%d",&choice);
		if(choice==1){
			strcpy(message.mtext, "shut down");
			printf("Sending Shutdown Request to Server\n");
			if(msgsnd(msgid, &message, sizeof(message.mtext), 0)==-1){
				perror("error in msgsnd()");
        			exit(1);
			}
			break;
		}
		else if(choice==0)
			continue;
		else{
			printf("Please enter a valid option\n");
			continue;
		}	
		
	}
	while(1){
		if(msgrcv(msgid, &message, sizeof(message.mtext), 99, 0)==-1){
        	perror("error in msgrcv()");
       		continue;	//to repeat msgrcv till a message is recieved
        	}
        	if(strcmp(message.mtext, "closed\n")==0){
        		printf("Server has been shut down\n");
        		break;	
        	}
	}
	return 0;
	}
	

