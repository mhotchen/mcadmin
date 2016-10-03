#ifndef MCADMIN_ACTIONS_H
#define MCADMIN_ACTIONS_H

#include <cdk/cdk.h>
#include "../memcache/commands.h"
#include "../ui/modals.h"
#include "../ui/screens.h"

enum ActionStatus {
    ACTION_STATUS_OK,
    ACTION_STATUS_NO_ACTION,
    ACTION_STATUS_ERROR
};

typedef struct Action Action;
struct Action {
    char key;
    enum ActionStatus (*execute)(CDKSCREEN *screen, int mcConn, Screen **currentScreen);
};

enum ActionStatus handleAction(int action, CDKSCREEN *screen, int mcConn, Screen **currentScreen);

#endif //MCADMIN_ACTIONS_H
