#include <netdb.h>
#include <stdbool.h>

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