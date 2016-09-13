#ifndef MCADMIN_CONNECT_H
#define MCADMIN_CONNECT_H

#include <errno.h>
#include <netdb.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

int connectByNetworkSocket(const char *const name, const char *const port);

#endif //MCADMIN_CONNECT_H
