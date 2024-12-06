#include <netdb.h>
#include <stdbool.h>
#include "ContentHandler.h"

#define BUFFER_SIZE 1024


typedef struct websocket
{
    struct addrinfo *addressInfo;
    int *socketId;
    bool *isConnected;
    WebSocketContent *content;

} Websocket;

Websocket* create_socket(const char *ip, const char *port);
/// Frees the socket
/// \param socket
void free_socket(Websocket* socket);
/// Closes the socket
/// \param socket
void close_socket(Websocket* socket);
/// Connects the websocket
/// \param socket Socket to use (see create_socket)
/// \return If success return true, otherwise false.
bool connect_socket(Websocket* socket);
/// Handles client requests
/// \param client_socket
void handle_client(Websocket* socket, int client_socket);
/// Handles the http packet and sends an http status code
/// \param packet packet to process
/// \return HTTP status code
int handle_http_packet(Websocket* socket, int client_socket, char *packet);
/// Send an response to the client socket
void client_response(int client_socket, int status_code, const char *phrase, const char *body);
