#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include "headers/webserver.h"
#include "headers/WebSocket.h"

int main(int argn, char *args[]) {
    if (argn != 3)
    {
        printf("Invalid arguments.\n");
        return 1;
    }

    const char *ip_adresse = args[1];
    const char *port_nummer = args[2];

    Websocket* server_socket = create_socket(ip_adresse, port_nummer);
    if (server_socket == NULL)
    {
        printf("Unable to create server socket!\n");
        return 2;
    }

    bool status = connect_socket(server_socket);
    printf("%b\n", status);

    return status;
}


