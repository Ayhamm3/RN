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
    const char *response = "Reply\r\n\r\n";
    long bytes_buffer;

    static char byte_cache[BUFFER_SIZE] = {0};
    char buffer_comb[BUFFER_SIZE * 2] = {0}; // Creates a new buffer to append the other bytes from receive of before in case of incompletion

    // Read bytes
    // As long as there are bytes receivable
    while ((bytes_buffer = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_buffer] = '\0'; // Isolate received bytes
        snprintf(buffer_comb, sizeof(buffer_comb), "%s%s", byte_cache, buffer); // write to buffer_combined

        char *start = buffer_comb;
        char *end;
        // Substring to iterate through packets received
        while ((end = strstr(start, "\r\n\r\n")) != NULL)
        {
            *end = '\0'; // null term to isolate packet
            printf("HTTP Paket received: %s\n", start);
            handle_http_packet(client_socket, start);
            start = end + 4; // Skip \r\n...
        }
        strcpy(byte_cache, start); // Copy to cache
    }

    if (bytes_buffer == -1)
    {
        printf("Error on receiving bytes\n");
        close(client_socket);
        return;
    }
    close(client_socket);
}

/// Handles the http packet and sends an http status code
/// \param packet packet to process
/// \return HTTP status code
int handle_http_packet(int client_socket, char *packet)
{
    char *line = strtok(packet, "\r\n"); // first line
    char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];

    //First line
    if (!line || (sscanf(line, "%s %s %s", method, uri, version) != 3))
    {
        client_response(client_socket, 400, "Bad Request");
        return 400;
    }

    printf("First line: method: %s uri: %s version: %s", method, uri, version);

    //Header
    // NULL here since strtok is already taking it from the first line request before
    bool areHeadersValid = true;
    while((line = strtok(NULL, "\r\n")))
    {
        const char *seperator = strchr(line, ':');
        if (!seperator || seperator < line || *(seperator + 1) == '\0')
        {
            areHeadersValid = false;
            break;
        }
    }
    
    if (!areHeadersValid)
    {
        client_response(client_socket, 400, "Bad Request");
        return 400;
    }

    // Handle the method
    if (strcmp(method, "GET") == 0)
    {
        client_response(client_socket, 404, "Not Found");
        return 404;
    }
    else
    {
        client_response(client_socket, 501, "Not Implemented");
        return 501;
    }
}

void client_response(int client_socket, int status_code, const char *phrase)
{
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s\r\n"
             "Content-Length: 0\r\n" // When adding http body later, need to replace this and adjust the method
             "Content-Type: text/plain\r\n\r\n",
             status_code, phrase);
    send(client_socket, response, strlen(response), 0);
    printf("Response sent: %s\n", response);
}
