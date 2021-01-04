#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "net.h"

#define BACKLOG 10 // how many pending connections queue will hold

/**
 * This gets an Internet address, either IPv4 or IPv6
 *
 * Helper function to make printing easier.
 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/**
 * Return the main listening socket
 *
 * Returns -1 or error
 */
int get_listener_socket(int port)
{
    int sockfd;
    struct sockaddr_in name;
    int sock_opt_val = 1;

    // af_inet allows us to bind to ipv4 addresses

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("(server_connection): socket() error");
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val,
                   sizeof(sock_opt_val)) < 0)
    {
        perror("(server_connection): Failed to set SO_REUSEADDR on INET socket");
        return -1;
    }

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
        perror("(server_connection): bind() error");
        return -1;
    }
    return sockfd;
}
