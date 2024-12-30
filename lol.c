#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

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

    // Read 1 byte from stdin into c, and keep doing it
    // until no more bytes to read, or c == q

    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {

        // Check if input is a nonprintable character
        // If yes, print ASCII code

        if (iscntrl(c)) {
            printf("%d\n", c);
        } else {
            printf("%d ('%c')\n", c, c);
        }
    }
    return 0;
}