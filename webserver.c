#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

int erstelle_socket(const char *ip, const char *port);
int warte_auf_verbindungen(int server_socket);

int main(int anzahl_args, char *argumente[]) {
    if (anzahl_args != 3) {
        return 1;
    }

    const char *ip_adresse = argumente[1];
    const char *port_nummer = argumente[2];

    int server_socket = erstelle_socket(ip_adresse, port_nummer);
    if (server_socket == -1) {
        return 2;
    }

    int status = warte_auf_verbindungen(server_socket);
    close(server_socket);

    return status;
}

//Erstellt und konfiguriert Socket
int erstelle_socket(const char *ip, const char *port) {
    struct addrinfo hinweise = {0}, *addr_info;
    hinweise.ai_family = AF_INET;
    hinweise.ai_socktype = SOCK_STREAM;
    hinweise.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, port, &hinweise, &addr_info) != 0) {
        return -1;
    }

    int socket_handle = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (socket_handle == -1) {
        freeaddrinfo(addr_info);
        return -1;
    }

    int option = 1;
    if (setsockopt(socket_handle, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
        close(socket_handle);
        freeaddrinfo(addr_info);
        return -1;
    }

    if (bind(socket_handle, addr_info->ai_addr, addr_info->ai_addrlen) == -1) {
        close(socket_handle);
        freeaddrinfo(addr_info);
        return -1;
    }

    freeaddrinfo(addr_info);
    return socket_handle;
}

int warte_auf_verbindungen(int server_socket) {
    if (listen(server_socket, 10) == -1) {
        return 3;//Fehler
    }

    while (1) {
        struct sockaddr_storage client_info;
        socklen_t client_info_groesse = sizeof(client_info);
        
        //Akzeptiert neue Verbindungen n shit
        int client_socket = accept(server_socket, (struct sockaddr *)&client_info, &client_info_groesse);
        if (client_socket == -1) {
            continue;//Fehler
        }

        //Verbindung erfolgreich
        close(client_socket);
    }

    return 0;
}
