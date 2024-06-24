#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_MSG_SIZE 256

struct msgbuf {
    long mtype;
    char mtext[MAX_MSG_SIZE];
};

void pingServer(int msgid, int clientID) {
    
    struct msgbuf message;
    message.mtype = clientID;	//setting mtype to be the client id of requesting client
    strcpy(message.mtext, "hello");	
    printf("Sent hello to client %d\n" , clientID);
    if(msgsnd(msgid, &message, sizeof(message.mtext), 0)==-1){
	perror("Error in msgsnd()");
	exit(1);
	}
    exit(0);
}

void FileSearch(int msgid, int clientID, char* filename) {

    struct msgbuf retmsg;
    retmsg.mtype = clientID;
    int status;
    int pipe_fd[2];
    if(pipe(pipe_fd) == -1){
   	perror("Error in creating Pipe");
    	exit(1);
    	}
    pid_t p1 = fork();
    
    if (p1 == -1) {
        perror("error in fork()");
        exit(1);
    }
 
    if(p1 == 0){				//Child process
	int nf;
    	close(pipe_fd[0]);
    	dup2(pipe_fd[1] , 1);			//duplicate stdout with write end of the pipe
    	close(pipe_fd[1]);		
	freopen("/dev/null", "w", stderr);	//suppessing stderr from server output
    	fflush(stdout);				//to remove garbage values

    	execlp("wc", "wc", "-w", filename ,NULL);  //Command output is dup from stdout to pipe
    	perror("execlp");

    	exit(1);

    }

    else{	//parent

    	waitpid(p1 , &status , 0);
    	close(pipe_fd[1]);

    	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) //this statement returns true if execlp
    							   //happened with no error ie it found a file
    		strcpy(retmsg.mtext, "Found");			
  	else
   		strcpy(retmsg.mtext, "Not Found");
    		
    	close(pipe_fd[0]);

    }  
	
    if(msgsnd(msgid, &retmsg, sizeof(retmsg.mtext), 0)==-1){
	perror("error in msgsnd()");
        exit(1);
	}
    printf("Sent \"File %s\" to client %d\n" ,retmsg.mtext , clientID);

    exit(0);
       
}

void WordCount(int msgid, int clientID, char* filename) {
    struct msgbuf retmsg;		//essentially same as previous, but
    					//returns output of execlp(wc -w)
    retmsg.mtype = clientID;
    int status;

    int pipe_fd[2];

    if(pipe(pipe_fd) == -1){
    	perror("Pipe");
    	exit(1);
    	}
    pid_t p1 = fork();

    if (p1 == -1) {
        perror("error in fork()");
        exit(1);
    }
    
    if(p1 == 0){
    	close(pipe_fd[0]);
    	dup2(pipe_fd[1] , 1);
    	close(pipe_fd[1]);
    	freopen("/dev/null", "w", stderr);
    	fflush(stdout);
    	execlp("wc", "wc", "-w", filename ,NULL);
    	perror("execlp");

    	exit(1);

    }

    else{
    	waitpid(p1 , &status , 0);
    	close(pipe_fd[1]);
    	
   	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		FILE* pipe_fp = fdopen(pipe_fd[0], "r");//created a file to read from pipe to 
							//store it in return message's mtext
        	fgets(retmsg.mtext, sizeof(retmsg.mtext), pipe_fp);	
        	//as fgets() expects a file 
        	fclose(pipe_fp);
        	retmsg.mtext[strlen(retmsg.mtext)-1] = ' ';
        } 
        else {
            strcpy(retmsg.mtext, "Not Found");
        }
   	close(pipe_fd[0]);
    }

    if(msgsnd(msgid, &retmsg, sizeof(retmsg.mtext), 0)==-1){
	perror("error in msgsnd()");
        exit(1);
	}    
    printf("Sent %s\b to client %d\n" ,retmsg.mtext , clientID);
    exit(0);
}

int main() {
	
	int closing_flag=0;	
        // Initialize message queue
        key_t key = ftok("client.c", 'C');
        if(key==-1){
		perror("error in ftok()");
        	exit(1);
	}
        int msgid = msgget(key, 0666 | IPC_CREAT);	//creates the message queue
        if (msgid == -1) {
        	perror("msgget");
        	exit(1);
        }
	
	fflush(stdout); 	//removing garbage values from buffer
	int arr[10]={0};	//array to store clientIDs which are currently active
	
    	while (1) {		//recieve messages in loop
	
        struct msgbuf message;

        if(msgrcv(msgid, &message, sizeof(message.mtext), 0, 0)==-1){
        	perror("error1");
       		continue;	//to repeat msgrcv till a message is recieved
        }
	
        int clientID = message.mtype;
        int choice = message.mtext[0] - '0';	//choice from client
        
        if(clientID==99){		//message from cleanup process
		printf("Shutdown request received. Waiting for all clients to exit\n");
		closing_flag=1;		//put server in closing state
		}
		
        if(clientID!=99){		//to log new client processes and remove them 
        				//after they send exit request
        	int i=0;
        	while(arr[i]!=0 && i<10){
        		if(arr[i]==clientID){
        			break;
        		}
        		i++;
        	}
        	if(arr[i]==0)
        		arr[i]=clientID;
        	if(choice==4){
        		printf("Client %d left\n",clientID);
        		for(int j=i;j<9;j++)
        			arr[j]=arr[j+1];
        		arr[9]=0;	
        	}
        }		

	if(closing_flag){			//Server Shutdown State
		if(arr[0]==0){			//no clients left	  						 
			while(wait(NULL) >= 0){}
			
			message.mtype=99;
			strcpy(message.mtext,"closed\n");
			if(msgsnd(msgid,&message,sizeof(message.mtext),0)==-1){
				perror("error in msgsnd()");
				exit(-1);
				}	
			if(msgctl(msgid,IPC_RMID,NULL)==-1){	//closing message queue
		           perror("Error in msgctl() in line 52");
		           exit(4);
	            	}
	            	printf("Shutting down\n");	
	            	exit(0);
	        }
	}
	
        pid_t p = fork(); 	//create child process to service request
        
        if(p == 0){		//only child executes functions

        	switch (choice) {
            		case 2:
                		FileSearch(msgid, clientID, message.mtext + 1 );
                		break;
            		case 3:
                		WordCount(msgid, clientID, message.mtext + 1);
                		break;
           		case 4:
            			exit(0);
               			break;
            		case 1:
            			pingServer(msgid, clientID);
                		break;	
                		
                	default:
                		exit(0);		
        		}        	
      		 
      		}             
	}
        return 0;
}


