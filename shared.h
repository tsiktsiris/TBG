////////////////////////////////////////////////////////////
//  Leitourgika Systhmata
//  Ergasia Eksamhnou - The Bidding Game
//  2013 - Dimitris Tsiktsiris
//  ---
//  Shared configuration file (same for server & client)
////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <assert.h>


//Error constants
#define E_NOCONNECTION      100
#define E_NOSOCKET          101
#define E_SENDFAILED        102
#define E_RECVFAILED        103
#define E_BINDFAILED        104
#define E_CONNREFUSED       105

#define E_NOTHREADING      106
#define E_FULL             107
#define E_RESUNKNOWN       108

#define RECV_BUFFER 50
#define PORT 8888


void send_all(int socket, void *buffer, size_t length)
{
    if(socket > 0)
    {
      size_t i = 0;
      for (i = 0; i < length;i += send(socket, buffer, length - i,0));
    }
}

int send_(int socket, char *message)
{
    send_all(socket , message , strlen(message));
}

int receive(int socket, char *buffer)
{
    size_t i = 0;
    for(i=0;i<RECV_BUFFER;i++) buffer[i] = 0;
    return recv(socket , buffer , RECV_BUFFER , 0);
}

char** str_split(char* a_str,const char a_delim) //Split a string based on a delimiter
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, &a_delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, &a_delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
