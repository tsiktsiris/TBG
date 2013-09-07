////////////////////////////////////////////////////////////
//  Leitourgika Systhmata
//  Ergasia Eksamhnou - The Bidding Game
//  2013 - Dimitris Tsiktsiris
//  ---
//  Game Client v0.1
////////////////////////////////////////////////////////////

#include "config.h"


void draw_obj(int pos)
{
    int x;

    printf("[P1]");

    for(x = 0 ; x < pos ; x++) printf("_ ");

    printf("O ");

    for(x=pos+1;x<9;x++) printf("_ ");

    printf("[P2]");
}

void move_cursor(int x, int y)
{
    printf("\033[%d;%dH",x,y);
}

void clear()
{
    system("clear");
}


void draw(int objpos, int balance, int advantage)
{
    clear();
    printf("Your Balance: %d \n",balance);
    move_cursor(0,30);
    printf("P%d has the advantage\n",advantage);
    move_cursor(5,0);
    draw_obj(objpos-1);
    move_cursor(10,0);
}


int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[50] , server_reply[RECV_BUFFER],nickname[20];
    char ** tokens;
    char ** subtokens;

    int i;

    setbuf(stdout, NULL);

    printf("The Bidding Game 2013 - Dimitris Tsiktsiris\n");
    printf("Game Client v0.1\n\n");

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Unable to create socket");
        return E_NET_NOSOCKET;
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return E_NET_NOCONNECTION;
    }

    printf("Enter your nickname: ");
    scanf("%s" , nickname);

    //keep communicating with server
    sprintf(message, "HELLO %s",nickname);
    send_(sock , message);

    for(i=0;i<RECV_BUFFER;i++) server_reply[i] = 0;

    recv(sock , server_reply , RECV_BUFFER , 0);
    tokens = str_split(server_reply,' ');

    int playerid = atoi(tokens[1]);

    if(strcmp(tokens[0],"WELCOME") == 0)
    {
        printf("Welcome %s, you are player %s\n",nickname,tokens[1]);
        printf("Wait for other players...\n");
    }
    else if (strcmp(tokens[0],"FULL") == 0)
    {
        printf("Sorry, server is full...\n");
        return 0;
    }
    else
    {
        printf("Unknown response from server (%s)\n",tokens[0]);
        return 0;
    }

    while(1)
    {
        for(i=0;i<RECV_BUFFER;i++) server_reply[i] = 0;


        //Receive a reply from the server
        if( recv(sock , server_reply , RECV_BUFFER , 0) < 0)
        {
            puts("recv failed");
            return E_NET_RECVFAILED;
        }

        tokens = str_split(server_reply,' ');

        if(strcmp(tokens[0],"STARTINGGAME") == 0)
        {
            printf("THE GAME IS STARTING!\n");
            clear();
            int balance = atoi(tokens[1]);
            draw(5,balance,1);
            while(1)
            {
               int bid = 0;
               printf("Enter your bid: ");
               scanf("%d",&bid);

               sprintf(message, "BID %d", bid);
               send_(sock , message);
               for(i=0;i<RECV_BUFFER;i++) server_reply[i] = 0;
               if( recv(sock , server_reply , RECV_BUFFER , 0) < 0)
               {
                 puts("recv failed");
                 return E_NET_RECVFAILED;
               }

               tokens = str_split(server_reply,' ');
               if(strcmp(tokens[0],"OKBID") == 0)
               {
                   printf("Bid has been placed!\n");
                   while(1)
                   {
                      for(i=0;i<RECV_BUFFER;i++) server_reply[i] = 0;
                      if( recv(sock , server_reply , RECV_BUFFER , 0) < 0)
                      {
                        puts("recv failed");
                        return E_NET_RECVFAILED;
                      }

                      tokens = str_split(server_reply,'#');

                      subtokens = str_split(tokens[0],' ');
                      char * objname = subtokens[0];
                      char * objfield =subtokens[1];

                      subtokens = str_split(tokens[1],' ');      char * p1field  = subtokens[1];
                      subtokens = str_split(tokens[2],' ');      char * p2field  = subtokens[1];
                      subtokens = str_split(tokens[3],' ');      char * advfield = subtokens[1];
                      subtokens = str_split(tokens[4],' ');      char * winfield = subtokens[1];

                     int objpos     = atoi(objfield);
                     int p1bid      = atoi(p1field);
                     int p2bid      = atoi(p2field);
                     int advantage  = atoi(advfield);
                     int winner     = atoi(winfield);

                     if(strcmp(objname,"OBJECT") == 0)
                      {
                          if(playerid==1) balance = balance - p1bid;
                          else if (playerid==2) balance = balance - p2bid;

                          printf("BIDS:\n");
                          printf("P1 bid: %d\n",p1bid);
                          printf("P2 bid: %d\n",p2bid);

                              if(playerid==winner)
                               printf("You won the round!\n");
                              else if (playerid!=winner)
                               printf("You lost the round.\n");

                          for(i=0;i<RECV_BUFFER;i++) server_reply[i] = 0;
                          //sleep(1);
                          int timeout = 1 , got = 0;

                          while(timeout>0)
                          {
                            sleep(1);
                            got= recv(sock , server_reply , RECV_BUFFER , MSG_DONTWAIT | MSG_PEEK);
                            if(got>0) break;
                            timeout--;
                          }


                          if(got > 0)
                          {
                            tokens = str_split(server_reply,' ');

                            if(strcmp(tokens[0],"WINNER") == 0)
                            {
                              printf("Winner is P%d!\n",atoi(tokens[1]));
                              exit(0);
                            }
                          }

                          sleep(1);
                          draw(objpos,balance,advantage);
                          break;
                      }


                   }
               }
               else if(strcmp(tokens[0],"INVALIDBID") == 0)
               {
                   printf("Invalid bid!\n");
               }

            }
        }
    }
    close(sock);
    return 0;
}
