#include "actions.h"

/**
 * Will invalidate all existing cache entries in the memcache server we're
 * connected to. Note that we may have lost a connection.
 */
static enum ActionStatus
flushAllContent(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    char *errorTextLines[1]     = {""};
    enum ActionStatus status    = ACTION_STATUS_OK;
    int               selection;

    {
        char *buttons[] = {"Yes", "No"};
        char *lines[]   = {"Are you sure you want to invalidate all entries in your memcache instance?"};

        selection = popup(screen, 1, lines, 2, buttons);
    }

    if (selection == 0) {
        switch (flushAll(mcConn)) {
            case MC_COMMAND_STATUS_SUCCESS:
                break;
            case MC_COMMAND_STATUS_LOST_CONNECTION:
                errorTextLines[0] = "Lost connection to memcache";
                break;
        }
    }

    if (strcmp(errorTextLines[0], "") != 0) {
        char *buttons[] = {"Close"};
        popup(screen, 1, errorTextLines, 1, buttons);
        status = ACTION_STATUS_ERROR;
    }

    return status;
}

/**
 * Search for, view, and/or delete a value in memcache by its key. This could
 * result in a lost connection, or a memory error. Other types of errors (eg.
 * item not found) are handled internally.
 */
static enum ActionStatus
searchForKey(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    char              key[256];
    char              *errorTextLines[1] = {""};
    Item              item               = {0};
    enum ActionStatus status             = ACTION_STATUS_OK;
    int               selection;

    textBox(screen, "Type the key to search for and press enter. Leave blank to cancel", "Key: ", key);
    if (key == 0 || strcmp(key, "") == 0) {
        return status;
    }

    switch (getItem(&item, key, mcConn)) {
        case MC_COMMAND_STATUS_SUCCESS:
            break;
        case MC_COMMAND_STATUS_ITEM_NOT_FOUND:
            errorTextLines[0] = "Item not found";
            break;
        case MC_COMMAND_STATUS_LOST_CONNECTION:
            errorTextLines[0] = "Lost connection to memcache";
            status = ACTION_STATUS_ERROR;
            break;
        case MC_COMMAND_STATUS_MEMORY_ERROR:
            errorTextLines[0] = "Unable to allocate memory";
            status = ACTION_STATUS_ERROR;
            break;
    }

    if (strcmp(errorTextLines[0], "") != 0) {
        char *buttons[] = {"Close"};
        popup(screen, 1, errorTextLines, 1, buttons);
        return status;
    }

    {
        char *buttons[] = {"View", "Delete", "Cancel"};
        char *lines[]   = {"What action would you like to take with this item?"};
        selection = popup(screen, 1, lines, 3, buttons);
    }

    switch (selection) {
        case 0: {
            /* 2 bytes for the 32bit flag, 14 for the extra characters below */
            char *title = calloc(sizeof(item.key) + 2 + 14, 0);
            sprintf(title, "Key: %s, flags: %d", item.key, item.flags);

            scrollPopup(screen, title, item.lines, item.value);
            free(title);
            break;
        }
        case 1: {
            char *buttons[] = {"Close"};
            char *lines[1];
            switch (deleteItem(key, mcConn)) {
                case MC_COMMAND_STATUS_SUCCESS:
                    lines[0] = "Deleted";
                    break;
                case MC_COMMAND_STATUS_ITEM_NOT_FOUND:
                    lines[0] = "Couldn't delete. Maybe the item doesn't exist anymore";
                    break;
                case MC_COMMAND_STATUS_LOST_CONNECTION:
                    lines[0] = "Lost connection to memcache";
                    status = ACTION_STATUS_ERROR;
                    break;
                case MC_COMMAND_STATUS_MEMORY_ERROR:
                    lines[0] = "Unable to allocate memory";
                    status = ACTION_STATUS_ERROR;
                    break;
            }

            popup(screen, 1, lines, 1, buttons);
            break;
        }
        case 2:
        default:
            break;
    }

    return status;
}

/**
 * Set action status for quitting application. Doesn't do any cleanup of its
 * own, it just uses this status to exit the main event loop.
 */
static enum ActionStatus
quit(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    return ACTION_STATUS_QUIT;
}

/**
 * Cycle between the screens (eg. to go from slabs to stats view).
 */
static enum ActionStatus
switchView(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    (*currentScreen) = (*currentScreen)->next;
    update_panels();

    return ACTION_STATUS_OK;
}

/**
 * Cycle between a screen's tabs (eg. to cycle through the slabs).
 */
static enum ActionStatus
switchTab(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    (*currentScreen)->currentPanel = (PANEL *) panel_userptr((*currentScreen)->currentPanel);
    top_panel((*currentScreen)->currentPanel);

    return ACTION_STATUS_OK;
}

/**
 * An action is a key that's attached to some action that may affect the
 * screen, and/or make one/several requests to memcache. This also includes
 * quitting the application.
 */
enum ActionStatus
handleAction(int action, CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    static Action actions[] = {
        {'f', &flushAllContent},
        {'/', &searchForKey},
        {'s', &switchView},
        { 9 , &switchTab},
        {'q', &quit}
    };
    int actionCount = sizeof(actions) / sizeof(actions[0]);

    for (int i = 0; i < actionCount; ++i) {
        if (actions[i].key == action) {
            return actions[i].execute(screen, mcConn, currentScreen);
        }
    }

    return ACTION_STATUS_NO_ACTION;
}
