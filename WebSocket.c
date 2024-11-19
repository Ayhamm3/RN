#include "headers/WebSocket.h"
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <string.h>

Websocket* create_socket(const char *ip, const char *port) {
    struct addrinfo hints = {0}, *addr_info;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, port, &hints, &addr_info) != 0)
    {
        printf("create_socket failed at getaddrinfo!\n");
        return NULL;
    }

    int socket_handle = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (socket_handle == -1)
    {
        freeaddrinfo(addr_info);
        printf("create_socket failed at socket_handle!\n");
        return NULL;
    }

    struct timeval timeout;
    timeout.tv_sec = 5; // 5 Sekunden timeout
    timeout.tv_usec = 0; // Keine Microsekunden
    if (setsockopt(socket_handle, SOL_SOCKET, SO_REUSEADDR, &timeout, sizeof(timeout)) == -1)
    {
        close(socket_handle);
        freeaddrinfo(addr_info);
        printf("create_socket failed at setsockopt!\n");
        return NULL;
    }

    if (bind(socket_handle, addr_info->ai_addr, addr_info->ai_addrlen) == -1)
    {
        close(socket_handle);
        freeaddrinfo(addr_info);
        printf("create_socket failed at bind!\n");
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
            printf("create_socket failed at allocating socketId!\n");
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
            printf("create_socket failed at allocating isConnected!\n");
            return NULL;
        }
        *websocket->isConnected = false;
    }
    else
    {
        close(socket_handle);
        freeaddrinfo(addr_info);
        printf("create_socket failed at allocating websocket!\n");
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
        printf("Calling close_socket but socket is not connected!\n");
        return;
    }
    if (socket->socketId == NULL)
    {
        printf("Calling close_socket but socketId is NULL!\n");
        return;
    }

    close(*(socket->socketId));
    *socket->isConnected = false;
}

bool connect_socket(Websocket* socket) {

    if (listen(*socket->socketId, 10) == -1)
    {
        printf("Failed to listen to the websocket!\n");
        return false;
    }
    *socket->isConnected = true;
    printf("Connected websocket on ws id %d\n", *socket->socketId);

    while (*socket->isConnected)
    {
        struct sockaddr_in client_info;
        socklen_t client_info_groesse = sizeof(client_info);

        // Accept new clients
        int client_socket = accept(*socket->socketId, (struct sockaddr *) &client_info, &client_info_groesse);
        if (client_socket == -1)
        {
            printf("Did not receive client\n");
            continue;
        }
        char *client_ip = inet_ntoa(client_info.sin_addr);
        int client_port = ntohs(client_info.sin_port);
        printf("Client connected: %s:%d\n", client_ip, client_port);

        handle_client(client_socket);
    }

    printf("Websocket disconnected\n");
    return true;
}

void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    long bytes_buffer;

    // Read bytes
    bytes_buffer = recv(client_socket, buffer, sizeof(buffer) -1, 0);
    if (bytes_buffer == -1)
    {
        printf("Did not receive shit\n");
        close(client_socket);
        return;
    }
    buffer[bytes_buffer] = '\0';
    printf("Message received: %s\n", buffer);

    const char *response = "Message received.";
    send(client_socket, response, strlen(response), 0);
    printf("Response sent: %s\n", response);

    close(client_socket);
}
