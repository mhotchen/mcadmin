#include "modals.h"

void
textBox(CDKSCREEN *screen, char *title, char *label, char userInput[256])
{
    CDKENTRY *textBox = newCDKEntry(
        screen,
        CENTER,
        CENTER,
        title,
        label,
        A_REVERSE,
        '_',
        vMIXED,
        60,
        0,
        256,
        true,
        false
    );

    memcpy(userInput, activateCDKEntry(textBox, 0), 256);
}

int
popup(CDKSCREEN *screen, int lineCount, char *lines[lineCount], int buttonCount, char *buttons[buttonCount])
{
    CDKDIALOG *diag = newCDKDialog(
        screen,
        CENTER,
        CENTER,
        lines,
        lineCount,
        buttons,
        buttonCount,
        A_REVERSE,
        true,
        true,
        false
    );

    return activateCDKDialog(diag, 0);
}

int
scrollPopup(CDKSCREEN *screen, char *title, int lineCount, char *lines[lineCount])
{
    CDKSCROLL *view = newCDKScroll(
        screen,
        CENTER,
        CENTER,
        RIGHT,
        80,
        80,
        title,
        lines,
        lineCount,
        true,
        A_REVERSE,
        true,
        false
    );

    return activateCDKScroll(view, 0);

}
