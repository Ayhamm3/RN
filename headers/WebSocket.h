#include <netdb.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024


typedef struct websocket
{
    struct addrinfo *addressInfo;
    int *socketId;
    bool *isConnected;

} Websocket;

Websocket* create_socket(const char *ip, const char *port);
void free_socket(Websocket* socket);
void close_socket(Websocket* socket);
/// Connects the websocket
/// \param socket Socket to use (see create_socket)
/// \return If success return true, otherwise false.
bool connect_socket(Websocket* socket);
void handle_client(int client_socket);
int handle_http_packet(int client_socket, char *packet);
void client_response(int client_socket, int status_code, const char *phrase);