#include "actions.h"

static void
flushAllContent(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    char *buttons[] = {"Yes", "No"};
    char *lines[] = {"Are you sure you want to invalidate all entries in your memcache instance?"};
    if (popup(screen, 1, lines, 2, buttons) == 0) {
        flushAll(mcConn);
    }
}

static void
searchForKey(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    char key[256];
    textBox(screen, "Type the key to search for and press enter. Leave blank to cancel", "Key: ", key);
    if (key == 0 || strcmp(key, "") == 0) {
        return;
    }

    Item item = {0};
    if (!getItem(&item, key, mcConn)) {
        char *buttons[] = {"Close"};
        char *lines[] = {"Item not found"};
        popup(screen, 1, lines, 1, buttons);
        return;
    }

    int selection;
    {
        char *buttons[] = {"View", "Delete", "Cancel"};
        char *lines[] = {"What action would you like to take with this item?"};
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
            char *lines[] = {
                deleteItem(key, mcConn)
                    ? "Deleted"
                    : "Couldn't delete. Maybe the item doesn't exist anymore"
            };
            popup(screen, 1, lines, 1, buttons);
            break;
        }
        case 2:
        default:
            break;
    }
}

static void
quit(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{

}

static void
switchView(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    (*currentScreen)->next->refreshData((*currentScreen)->next, mcConn);
    (*currentScreen) = (*currentScreen)->next;
    update_panels();
}

static void
switchTab(CDKSCREEN *screen, int mcConn, Screen **currentScreen)
{
    (*currentScreen)->currentPanel = (PANEL *) panel_userptr((*currentScreen)->currentPanel);
    top_panel((*currentScreen)->currentPanel);
}

void
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
}
