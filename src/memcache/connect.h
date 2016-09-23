#ifndef MCADMIN_CONNECT_H
#define MCADMIN_CONNECT_H

#include <errno.h>
#include <netdb.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

enum McConnectStatus {
    MC_CONN_STATUS_SUCCESS,
    MC_CONN_STATUS_LOOKUP_ERROR,
    MC_CONN_STATUS_CANT_CONNECT
};

enum McConnectStatus connectByNetworkSocket(const char *const name, const char *const port, int *sockfd);

#endif //MCADMIN_CONNECT_H
