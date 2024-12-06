#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE

#include "headers/WebSocket.h"
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>


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
            free_socket(websocket);
            printf("create_socket failed at allocating socketId!\n");
            return NULL;
        }
        *websocket->socketId = socket_handle;

        // Alloc isConnected
        websocket->isConnected = (bool*) malloc(sizeof(bool));
        if (websocket->isConnected == NULL)
        {
            close(socket_handle);
            free_socket(websocket);
            printf("create_socket failed at allocating isConnected!\n");
            return NULL;
        }
        *websocket->isConnected = false;

        websocket->content = create_content();
        if (websocket->content == NULL)
        {
            close(socket_handle);
            free_socket(websocket);
            printf("create_socket failed at allocating content!\n");
            return NULL;
        }
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
        close_socket(socket);
        free(socket->socketId);
    }
    if (socket->isConnected != NULL)
        free(socket->isConnected);
    if (socket->addressInfo != NULL)
        freeaddrinfo(socket->addressInfo);
    if (socket->content != NULL)
        free(socket->content);

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

        handle_client(socket, client_socket);
    }

    printf("Websocket disconnected\n");
    return true;
}

void handle_client(Websocket* socket, int client_socket)
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
            handle_http_packet(socket, client_socket, start);
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

int handle_http_packet(Websocket* socket, int client_socket, char *packet)
{
    char *line = strtok(packet, "\r\n"); // first line
    char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];
    char body[BUFFER_SIZE] = {0};


    if (!line || (sscanf(line, "%s %s %s", method, uri, version) != 3))
    {
        client_response(client_socket, 400, "Bad Request", NULL);
        return 400;
    }

    printf("First line: method: %s uri: %s version: %s\n", method, uri, version);

    bool areHeadersValid = true;
    int contentLen = 0;
    while ((line = strtok(NULL, "\r\n")))
    {
        const char *separator = strchr(line, ':');
        if (!separator || separator < line || *(separator + 1) == '\0')
        {
            areHeadersValid = false;
            break;
        }

        if (strncasecmp(line, "Content-Length:", 15) == 0)
        {
            contentLen = atoi(line + 15);
        }
    }

    if (!areHeadersValid)
    {
        client_response(client_socket, 400, "Bad Request", NULL);
        return 400;
    }

    if (contentLen > 0)
    {
        char *body_start = packet + strlen(packet) + 2;
        strncpy(body, body_start, contentLen);
        body[contentLen] = '\0';
        printf("Body: %s\n", body);
    }

    // Split the URI into segments
    char *segments[BUFFER_SIZE];
    int segment_count = 0;
    char *token = strtok(uri, "/");
    while (token != NULL && segment_count < BUFFER_SIZE)
    {
        segments[segment_count++] = token;
        token = strtok(NULL, "/");
    }

    // Handle the HTTP method
    if (strcmp(method, "GET") == 0)
    {
        if (segment_count <= 0)
        {
            client_response(client_socket, 404, "Not Found", NULL);
            return 404;
        }

        if (strcmp(segments[0], "static") == 0)
        {
            const char *path = segments[1];
            if (strcmp(path, "foo") == 0)
            {
                client_response(client_socket, 200, "OK", "Foo");
                return 200;
            }
            else if (strcmp(path, "bar") == 0)
            {
                client_response(client_socket, 200, "OK", "Bar");
                return 200;
            }
            else if (strcmp(path, "baz") == 0)
            {
                client_response(client_socket, 200, "OK", "Baz");
                return 200;
            }
            else
            {
                client_response(client_socket, 404, "Not Found", NULL);
                return 404;
            }
        }
        else if (strcmp(segments[0], "dynamic") == 0)
        {
            char* content = get_content(socket->content, segments[1]);
            if (content == 0)
            {
                client_response(client_socket, 404, "Not Found", NULL);
                return 404;
            }
            else
            {
                client_response(client_socket, 200, "OK", content);
                return 200;
            }
        }
        else
        {
            client_response(client_socket, 404, "Not Found", NULL);
            return 404;
        }
    }
    else if (strcmp(method, "PUT") == 0)
    {
        if (segment_count <= 0)
        {
            client_response(client_socket, 404, "Not Found", NULL);
            return 404;
        }

        if (strcmp(segments[0], "dynamic") == 0)
        {
            int code = set_content(socket->content, segments[1], body);
            if (code == 201)
            {
                client_response(client_socket, 201, "Created", NULL);
                return 201;
            }

            if (code == 204)
            {
                client_response(client_socket, 204, "No Content", NULL);
                return 204;
            }

            if (code == 500)
            {
                client_response(client_socket, 500, "Internal Server Error", NULL);
                return 500;
            }
        }
        else
        {
            client_response(client_socket, 403, "Forbidden", NULL);
            return 403;
        }
    }
    else if (strcmp(method, "DELETE") == 0)
    {
        if (segment_count <= 0)
        {
            client_response(client_socket, 404, "Not Found", NULL);
            return 404;
        }

        if (strcmp(segments[0], "dynamic") == 0)
        {
            int code = delete_content(socket->content, segments[1]);
            if (code == 204)
            {
                client_response(client_socket, 204, "No Content", NULL);
                return 204;
            }
            else if (code == 404)
            {
                client_response(client_socket, 404, "Not Found", NULL); 
                return 404;
            }
        }
        else
        {
            client_response(client_socket, 403, "Forbidden", NULL);
            return 403;
        }
    }
    client_response(client_socket, 501, "Not Implemented", NULL);
    return 501;
}

void client_response(int client_socket, int status_code, const char *phrase, const char *body)
{
    char response[BUFFER_SIZE];
    size_t body_length = 0;
    if (body != NULL)
        body_length = strlen(body);
    else
        body = "";

    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s\r\n"
             "Content-Length: %zu\r\n"
             "Content-Type: text/plain\r\n\r\n"
             "%s",
             status_code, phrase, body_length, body);
    send(client_socket, response, strlen(response), 0);
    printf("Response sent: %s\n", response);
}
