////////////////////////////////////////////////////////////
//  Leitourgika Systhmata
//  Ergasia Eksamhnou - The Bidding Game
//  2013 - Dimitris Tsiktsiris
//  ---
//  Game Server (threaded) v0.1
////////////////////////////////////////////////////////////


#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <string.h>

#include "config.h"

#define MAX_STRING_LEN 80
#define CLIENTS 2

typedef enum { false, true } bool;

struct clientdata
{
    int clientid ;
    int bid      ;
    int balance  ;
    int position ;
    bool isready ;

    char nickname[20];
};

struct clientdata PLAYER[CLIENTS-1];

void *connection_handler(void *);
void *game_progress(void *);

int send_(int clientid, char *message)
{
   if(clientid != 0)
    {
      write(clientid , message , strlen(message));
      return 0;
    }
    else
    return -1;
}

int CLIENTS_CONNECTED = 0;
int CLIENTS_READY = 0;
int TH_STATUS = 0;

bool GAME_RUNNING = false;

int main(int argc , char *argv[])
{
    int idx, socket_desc, client_sock , c , *new_sock;
    struct sockaddr_in server , client;

    int starting_bid = 100;

    printf("The Bidding Game 2013 - Dimitris Tsiktsiris\n");
    printf("Game Server (threaded) v0.1\n\n");

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    if (socket_desc == -1)
    {
        printf("Unable to create socket");
        return E_NET_NOSOCKET;
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("Bind failed. Error");
        return E_NET_BINDFAILED;
    }

    //Listen
    listen(socket_desc , 3);

    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);


    //Accept and incoming connection
    c = sizeof(struct sockaddr_in);



    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        if(CLIENTS_CONNECTED < CLIENTS)
        {
            printf("[%d] Connection accepted from %s\n",client_sock,"mpam");
            pthread_t sniffer_thread;
            new_sock = malloc(1);
            *new_sock = client_sock;

            CLIENTS_CONNECTED++;

            if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
            {
                perror("could not create thread");
                return 1;
            }
            //Now join the thread , so that we dont terminate before the thread
            //pthread_join( sniffer_thread , NULL);

            if (client_sock < 0)
            {
                perror("accept failed");
                return E_NET_CONNREFUSED;
            }


        }

        if (CLIENTS_CONNECTED == CLIENTS)
        {
            if(GAME_RUNNING == false)
            {
                pthread_t game_thread;
                sleep(1);
                if( pthread_create( &game_thread , NULL ,  game_progress, (void*) starting_bid) < 0)
                {
                    perror("could not create game thread");
                    return 1;
                }
            }
            else send_(client_sock,"FULL");
        }



    }
    return 0;
}


void *game_progress(void *bid)
{
    GAME_RUNNING = true;
    while(CLIENTS_READY < CLIENTS_CONNECTED) sleep(1);
    printf("All players are ready! Creating game...");

    int idx;

    for(idx=0;idx<CLIENTS;idx++)
     send_(PLAYER[idx].clientid,"STARTINGGAME 100");
    printf("done!\n");

    while (GAME_RUNNING)
    {
        if(CLIENTS_READY != CLIENTS)
        {
            printf("The game has been abandoned! (player left)\n");
            GAME_RUNNING = false;
            return;
        }





    }

}


/*
 * This will handle connection for each client
 * */

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size, i;
    char message[50] , client_message[RECV_BUFFER];
    char **tokens;

    //Receive a message from client
    read_size = recv(sock , client_message , RECV_BUFFER , 0);


    tokens = str_split(client_message,' ');


    if(strcmp(tokens[0],"HELLO") == 0)
    {
        PLAYER[CLIENTS_CONNECTED-1].clientid = sock;
        PLAYER[CLIENTS_CONNECTED-1].bid = 0;
        PLAYER[CLIENTS_CONNECTED-1].balance = 100;
        strncpy(PLAYER[CLIENTS_CONNECTED-1].nickname,tokens[1],20);
        PLAYER[CLIENTS_CONNECTED-1].position = 0;
        //PLAYER[CLIENTS_CONNECTED-1].isready = false;
    }

    CLIENTS_READY++;

    sprintf(message, "WELCOME %d", CLIENTS_READY);
    send_(sock,message);

    while(1)
    {
        for(i=0;i<RECV_BUFFER;i++) client_message[i] = 0;

        read_size = recv(sock , client_message , RECV_BUFFER , 0);

        if(read_size > 0)
        {
          tokens = str_split(client_message,' ');

            if(strcmp(tokens[0],"BID") == 0)
            {
                int bid = atoi(tokens[1]);

                if((bid>0) && (bid<=PLAYER[CLIENTS_CONNECTED-1].balance))
                {
                    PLAYER[CLIENTS_CONNECTED-1].bid = bid;
                    PLAYER[CLIENTS_CONNECTED-1].balance = PLAYER[CLIENTS_CONNECTED-1].balance - bid;
                    PLAYER[CLIENTS_CONNECTED-1].isready = true;
                    printf("client placed valid bid: %d\n",bid);
                    send_(sock,"OKBID");
                }
                else
                {
                    printf("client placed invalid bid: %d\n",bid);
                    send_(sock,"INVALIDBID");
                }


            }
        }


        if(read_size == 0)
        {
            printf("[%d] Client disconnected\n",sock);
            fflush(stdout);
            break;
        }
        else if(read_size == -1)
        {
            printf("[%d] recv failed\n",sock);
            perror("Reason");
        }
    }

    //Free the socket pointer
    free(socket_desc);

    PLAYER[CLIENTS_CONNECTED-1].isready = false;
    CLIENTS_CONNECTED--;
    CLIENTS_READY--;

    return 0;
}
