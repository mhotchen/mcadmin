#include "connect.h"

/**
 * Uses getaddrinfo, socket, and connect to connect to a memcache host. This
 * could fail at the name lookup or connect phase.
 */
enum McConnectStatus
connectByNetworkSocket(const char *const name, const char *const port, int *sockfd) {
    struct addrinfo hints  = {0};
    struct addrinfo *info  = 0;
    int             conn   = -1;
    int             status = 0;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(name, port, &hints, &info);
    if (status != 0) {
        return MC_CONN_STATUS_LOOKUP_ERROR;
    }

    for (; info != NULL; info = info->ai_next) {
        *sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (*sockfd == -1) {
            continue;
        }

        conn = connect(*sockfd, info->ai_addr, info->ai_addrlen);
        if (conn == -1) {
            continue;
        }

        break;
    }

    if (*sockfd == -1 || conn == -1) {
        return MC_CONN_STATUS_CANT_CONNECT;
    }

    return MC_CONN_STATUS_SUCCESS;
}
