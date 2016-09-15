#include "connect.h"

int
connectByNetworkSocket(const char *const name, const char *const port) {
    struct addrinfo hints = {0};
    struct addrinfo *info = 0;

    int conn = -1;
    int mcConn = -1;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(name, port, &hints, &info);
    if (status != 0) {
        perror(gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    for (; info != NULL; info = info->ai_next) {
        mcConn = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (mcConn == -1) {
            continue;
        }

        conn = connect(mcConn, info->ai_addr, info->ai_addrlen);
        if (conn == -1) {
            continue;
        }

        break;
    }

    if (mcConn == -1 || conn == -1) {
        perror(strerror(errno));
        exit(EXIT_FAILURE);
    }

    return mcConn;
}
