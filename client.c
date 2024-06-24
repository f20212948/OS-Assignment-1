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
        perror("No message queue found, check if server is online");
        exit(1);
    }

    int clientID;
    printf("Enter Client-ID: ");
    scanf("%d", &clientID);

    int choice = 0;

    while (choice != 4) {
        printf("Menu:\n");
        printf("1. Enter 1 to Ping Server\n");
        printf("2. Enter 2 to do File Search\n");
        printf("3. Enter 3 do File Word Count\n");
        printf("4. Enter 4 to exit\n");

        scanf("%d", &choice);
	if(choice>=5 && choice<=0){
	  printf("Invalid choice\n");
	  continue;
	  }

        struct msgbuf message;
        message.mtype = clientID;	//to ensure only the message meant for this client is read by it

        switch (choice) {
            case 1:
                // Send "hi" to the server
                strcpy(message.mtext, "1hi");
                if(msgsnd(msgid, &message, sizeof(message.mtext), 0)==-1){
			perror("Cannot send message");
        		exit(1);
		}

                // Receive "hello" from the server (in a loop)

                while (1) {
                    if(msgrcv(msgid, &message, sizeof(message.mtext), clientID, 0)==-1){
			perror("error in msgrcv()");
        		exit(1);
			}
                    if (strcmp(message.mtext, "hello") == 0) {
                        printf("Received: %s\n", message.mtext);
                        break;
                    }
                }
                break;

            case 2:              
                strcpy(message.mtext , "2");
                char filename1[256];

                printf("Enter filename to search: ");
                scanf("%s", filename1);
                strcat(message.mtext , filename1);                

                if(msgsnd(msgid, &message, sizeof(message.mtext), 0)==-1){
			perror("error in msgsnd()");
        		exit(1);
		}

		while (1) {
                    if(msgrcv(msgid, &message, sizeof(message.mtext), clientID, 0)==-1){
			perror("error in msgrcv()");
        		exit(1);
			}
                    if (strcmp(message.mtext, "") >= 0) {
                        printf("File %s by server\n", message.mtext);
                        break;
                    }
                }
                break;
                
            case 3:
                strcpy(message.mtext , "3");
                printf("Enter filename for word count: ");

                char filename[256];
                scanf("%s", filename);
                strcat(message.mtext , filename);                
                if(msgsnd(msgid, &message, sizeof(message.mtext), 0)==-1){
			perror("error in msgsnd()");
        		exit(1);
			}
                
                while (1) {
                    if(msgrcv(msgid, &message, sizeof(message.mtext), clientID, 0)==-1){
			perror("error in msgrcv()");
        		exit(1);
			}
                    if (strcmp(message.mtext, "Not Found") == 0) {
                        printf("File %s\n", message.mtext);
                        break;
                    }
                    else if(strcmp(message.mtext, "") > 0){                    
                    	char *res = strtok(message.mtext , " ");
                    	printf("File has %s words\n" , res);
                    	break;
                    }
                }
                break;
                
            case 4:
             	strcpy(message.mtext , "4");
             	if(msgsnd(msgid, &message, sizeof(message.mtext), 0)==-1){
			perror("error in msgsnd()");
        		exit(1);
		}
		exit(0);		
                break;
            default:
                printf("Invalid choice\n");
		
        }

    }



    return 0;

}


