#include "headers/WebSocket.h"
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>

Websocket* create_socket(const char *ip, const char *port) {
    struct addrinfo hints = {0}, *addr_info;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, port, &hints, &addr_info) != 0)
    {
        printf("create_socket failed at getaddrinfo!");
        return NULL;
    }

    int socket_handle = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (socket_handle == -1)
    {
        freeaddrinfo(addr_info);
        printf("create_socket failed at socket_handle!");
        return NULL;
    }

    int option = 1;
    if (setsockopt(socket_handle, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
    {
        close(socket_handle);
        freeaddrinfo(addr_info);
        printf("create_socket failed at setsockopt!");
        return NULL;
    }

    if (bind(socket_handle, addr_info->ai_addr, addr_info->ai_addrlen) == -1)
    {
        close(socket_handle);
        freeaddrinfo(addr_info);
        printf("create_socket failed at bind!");
        return NULL;
    }

    Websocket* websocket = (Websocket*) malloc(sizeof(Websocket));
    if (websocket != NULL)
    {
        websocket->addressInfo = addr_info;
        
        // Alloc socketId
        websocket->socketId = (int*) malloc(sizeof(int));
        if (websocket->socketId == NULL)
        {
            close(socket_handle);
            freeaddrinfo(addr_info);
            free(websocket);
            printf("create_socket failed at allocating socketId!");
            return NULL;
        }
        *websocket->socketId = socket_handle;

        // Alloc isConnected
        websocket->isConnected = (bool*) malloc(sizeof(bool));
        if (websocket->isConnected == NULL)
        {
            close(socket_handle);
            freeaddrinfo(addr_info);
            free(websocket->socketId);
            free(websocket);
            printf("create_socket failed at allocating isConnected!");
            return NULL;
        }
        *websocket->isConnected = false;
    }
    else
    {
        close(socket_handle);
        freeaddrinfo(addr_info);
        printf("create_socket failed at allocating websocket!");
        return NULL;
    }

    return websocket;
}

void free_socket(Websocket* socket)
{
    if (socket == NULL) return;
    if (socket->socketId != NULL)
    {
        close(*(socket->socketId));
        free(socket->socketId);
    }
    if (socket->isConnected != NULL)
        free(socket->isConnected);
    if (socket->addressInfo != NULL)
        freeaddrinfo(socket->addressInfo);

    free(socket);
}

void close_socket(Websocket* socket)
{
    if (socket == NULL) return;
    if (*socket->isConnected == false)
    {
        printf("Calling close_socket but socket is not connected!");
        return;
    }
    if (socket->socketId == NULL)
    {
        printf("Calling close_socket but socketId is NULL!");
        return;
    }

    close(*(socket->socketId));
    *socket->isConnected = false;
}

bool connect_socket(Websocket* socket) {

    if (listen(*socket->socketId, 10) == -1)
    {
        printf("Failed to listen to the websocket!");
        return false;
    }
    *socket->isConnected = true;

    while (*socket->isConnected == false)
    {
        struct sockaddr_storage client_info;
        socklen_t client_info_groesse = sizeof(client_info);

        //Akzeptiert neue Verbindungen n shit
        int client_socket = accept(*socket->socketId, (struct sockaddr *)&client_info, &client_info_groesse);
        if (client_socket == -1)
        {
            printf("Failed to accept sockets!");
            return false;
        }
    }

    printf("Connected websocket on ws id %d", *socket->socketId);
    return true;
}
