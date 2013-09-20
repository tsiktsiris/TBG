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

#include "shared.h"

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

int CLIENTS_CONNECTED = 0;  //Connected clients
int CLIENTS_READY = 0;  //Players ready for game start (with nickname)
int TH_STATUS = 0; //Thread status
int OBJECTPOS = 4; //The object position (4 is the center)

bool GAME_RUNNING = false;

int main(int argc , char *argv[])
{
    int idx, socket_desc, client_sock , c , *new_sock;
    struct sockaddr_in server , client;

    int starting_bid = 100; //Defines the starting money for the players

    printf("The Bidding Game 2013 - Dimitris Tsiktsiris\n");
    printf("Game Server (threaded) v0.1\n\n");

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    if (socket_desc == -1)
    {
        printf("Unable to create socket");
        return E_NOSOCKET;
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
        return E_BINDFAILED;
    }

    //Listen
    listen(socket_desc , 3);

    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    //Accept incoming connections
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        if(CLIENTS_CONNECTED < CLIENTS)
        {
            printf("[%d] Connection accepted\n",client_sock);
            pthread_t sniffer_thread;
            new_sock = malloc(1);
            *new_sock = client_sock;

            CLIENTS_CONNECTED++;

            //Create a thread for the new client
            if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
            {
                perror("could not create thread");
                return 1;
            }

            if (client_sock < 0)
            {
                perror("accept failed");
                return E_CONNREFUSED;
            }
        }

        if (CLIENTS_CONNECTED == CLIENTS) //Check if connected clients are enough to start a game
        {
            if(GAME_RUNNING == false)
            {
                pthread_t game_thread;
                sleep(1);
                if( pthread_create( &game_thread , NULL ,  game_progress, (void*) starting_bid) < 0) //Create game thread
                {
                    perror("could not create game thread");
                    return 1;
                }
            }
            else send_(client_sock,"FULL"); //Inform client that server is full
        }
    }
    return 0;
}


void *game_progress(void *bid) //Game progress thread
{
    GAME_RUNNING = true;
    while(CLIENTS_READY < CLIENTS_CONNECTED) sleep(1); //Check if clients are ready
    printf("All players are ready! Creating game...");

    int ADVANTAGE = 0;
    size_t idx;
    char message[50];
    sleep(1);
    for(idx=0;idx<CLIENTS;idx++)
     send_(PLAYER[idx].clientid,"STARTINGGAME 100"); //Inform clients for the starting money

    printf("done!\n");

    while (GAME_RUNNING) //Game core loop
    {
        if(CLIENTS_READY != CLIENTS) //Check if players are still connected, if not destroy the game
        {
            printf("The game has been abandoned! (player left)\n");
            GAME_RUNNING = false;
            return;
        }

       //Wait for players to place a bid
       bool bidready=false;

       for(idx=0;idx<CLIENTS;idx++)
       {
           if(PLAYER[idx].isready==false)
           {
               bidready = true;
               break;
           }
       }

       //Processing loop
       if(bidready==false)
       {
         printf("Processing results...\n");

         //Find the maximum bid
         int winnerbid = 0;

         for(idx=0;idx<CLIENTS;idx++)
         {
             PLAYER[idx].isready=false;
             if(PLAYER[idx].bid >winnerbid )
              winnerbid = PLAYER[idx].bid;
            else if (PLAYER[idx].bid == winnerbid)
              winnerbid = -1;
         }

         //Find the winner of the round
         int roundwinner = -1;

         if(winnerbid>0)
         {
            for(idx=0;idx<CLIENTS;idx++)
            {
                if(PLAYER[idx].bid == winnerbid )
                {
                 PLAYER[idx].position--;
                 roundwinner = idx;
                }
                else
                 PLAYER[idx].position++;
            }
         }
         else
         {
             roundwinner=ADVANTAGE;
             for(idx=0;idx<CLIENTS;idx++)
             {
                 if(idx==roundwinner)
                 PLAYER[idx].position--;
                 else
                 PLAYER[idx].position++;
             }
         }

         OBJECTPOS = PLAYER[0].position;

         printf("Announcing...\n");
         sleep(2);
         //Announce the results to clients
         sprintf(message, "OBJECT %d#P1 %d#P2 %d#ADVANTAGE %d#WIN %d",OBJECTPOS,PLAYER[0].bid,PLAYER[1].bid,ADVANTAGE+1,roundwinner+1);

        //Writes scores to a file
        FILE *file;
        file = fopen("scores.txt","a+");
        for(idx=0;idx<CLIENTS;idx++)
        fprintf(file,"%s placed bid: %d \n",PLAYER[idx].nickname,PLAYER[idx].bid);
        fclose(file);

         for(idx=0;idx<CLIENTS;idx++)
         {
             send_(PLAYER[idx].clientid , message);
             PLAYER[idx].isready=false;
         }

         //If there is a winner, inform the clients
         bool winner = false;

         for(idx=0;idx<CLIENTS;idx++)
         {
             if(PLAYER[idx].position == 0 )
             {
                 winner = true;
                 break;
             }
         }

         if(winner == true)
         {
           //Write the winner to file
           FILE *file;
           file = fopen("scores.txt","a+");
           fprintf(file,"Player %s won! \n",PLAYER[idx].nickname);
           fclose(file);

           //Inform the clients for the winner
           sprintf(message, "WINNER %d",idx+1);
           for(idx=0; idx<CLIENTS; idx++)
           send_(PLAYER[idx].clientid,message);

           printf("Winner is P%d !\n", idx+1);
           printf("Destroying game...\n");
           break;
         }

        ADVANTAGE++;
        if(ADVANTAGE>=2) ADVANTAGE = 0;
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
    int read_size;
    size_t i;
    char message[50] , client_message[RECV_BUFFER];
    char **tokens;

    //Receive the handshake from client
    read_size = receive(sock , client_message);

    tokens = str_split(client_message,' ');

    int CURRENT = CLIENTS_CONNECTED-1;

    //Initialize player fields
    if(strcmp(tokens[0],"HELLO") == 0)
    {
        PLAYER[CURRENT].clientid = sock;
        PLAYER[CURRENT].bid = 0;
        PLAYER[CURRENT].balance = 100;
        strncpy(PLAYER[CURRENT].nickname,tokens[1],20);
        PLAYER[CURRENT].position = 4;
        PLAYER[CURRENT].isready = false;
    }

    CLIENTS_READY++;

    //Welcome the player
    sprintf(message, "WELCOME %d", CLIENTS_READY);
    send_(sock,message);

    //Find the client index based on the socket
    for(i=0;i<CLIENTS_CONNECTED;i++)
    {
      if(PLAYER[i].clientid==sock)
      {
        CURRENT=i;
        break;
       }
    }

    //Loop for checking & placing bids
    while(1)
    {
        read_size = receive(sock , client_message);

        if(read_size > 0)
        {
          tokens = str_split(client_message,' ');

            if(strcmp(tokens[0],"BID") == 0)
            {
                int bid = atoi(tokens[1]);

                if((bid>0) && (bid<=PLAYER[CURRENT].balance) && GAME_RUNNING == true)
                {
                    PLAYER[CURRENT].bid = bid;
                    PLAYER[CURRENT].balance = PLAYER[CURRENT].balance - bid;

                    printf("Player %d placed valid bid: %d\n",CURRENT,bid);
                    send_(PLAYER[CURRENT].clientid,"OKBID");

                    PLAYER[CURRENT].isready = true;
                }
                else if (GAME_RUNNING == false)
                {
                    for(i=0;i<CLIENTS_READY;i++) send_(PLAYER[i].clientid,"QUIT");
                }
                else
                {
                    printf("client placed invalid bid: %d\n",bid);
                    send_(PLAYER[CURRENT].clientid,"INVALIDBID");
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
