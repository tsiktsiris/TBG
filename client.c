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



int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[RECV_BUFFER],nickname[20];
    char ** tokens;


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
    send(sock , message , strlen(message) ,0);
    //sleep(100);
    if( recv(sock , server_reply , RECV_BUFFER , 0) < 0)
    {
            puts("recv failed");
            return E_NET_RECVFAILED;
    }
    tokens = str_split(server_reply,' ');

    if(strcmp(tokens[0],"WELCOME") == 0)
    {
        printf("Welcome %s, you are player %s\n",nickname,tokens[1]);
        printf("Please wait for other players...\n");
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
            break;
        }

        tokens = str_split(server_reply,' ');

        if(strcmp(tokens[0],"STARTINGGAME") == 0)
        {
            printf("THE GAME IS STARTING!\n");
            clear();
            printf("Your Balance: %s \n",tokens[1]);
            move_cursor(5,0);
            draw_obj(4);
            move_cursor(10,0);

            int bid = 0;
            printf("Enter your bid: ");
            scanf("%d",&bid);

            sprintf(message, "BID %d", bid);
            send(sock , message , strlen(message) ,0);




        }

    }

    close(sock);
    return 0;
}
