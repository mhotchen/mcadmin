#ifndef MCADMIN_MODALS_H
#define MCADMIN_MODALS_H

#include <cdk/cdk.h>

void textBox(CDKSCREEN *screen, char *title, char *label, char userInput[256]);
int  popup(CDKSCREEN *screen, int lineCount, char *lines[lineCount], int buttonCount, char *buttons[buttonCount]);
int  scrollPopup(CDKSCREEN *screen, char *title, int lineCount, char *lines[lineCount]);

#endif //MCADMIN_MODALS_H
