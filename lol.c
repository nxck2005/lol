#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>

// State of terminal originally
struct termios original_termios;

void disableRawMode() {
    tcsetattr(STDERR_FILENO, TCSAFLUSH, &original_termios);
}

void enableRawMode() {
    tcgetattr(STDERR_FILENO, &original_termios);
    atexit(disableRawMode);

    /* 
       ECHO : bitflag/field 
       0000000000000000000000000000100 
    */

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


int main() {
    enableRawMode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}