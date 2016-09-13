#ifndef MCADMIN_ACTIONS_H
#define MCADMIN_ACTIONS_H

#include <cdk/cdk.h>
#include "../memcache/commands.h"
#include "../ui/modals.h"
#include "../ui/screens.h"

typedef struct Action Action;
struct Action {
    char key;
    void (*execute)(CDKSCREEN *screen, int mcConn, Screen **currentScreen);
};

void handleAction(int action, CDKSCREEN *screen, int mcConn, Screen **currentScreen);

#endif //MCADMIN_ACTIONS_H
