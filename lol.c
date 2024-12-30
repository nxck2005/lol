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

    // Create copy of original terminal flags
    struct termios raw = original_termios;

    // Disable Ctrl S and Ctrl Q (software flow control)
    // and carriage return to make sure Ctrl M is read properly, and enter too
    raw.c_iflag &= ~(ICRNL | IXON);

    // Turn off all output postprocessing, like adding of /r after /n automatically
    raw.c_oflag &= ~(OPOST);

    // Turn off echo, canonical mode, Ctrl V, and sigterms
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

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
        // If not, print ASCII code with reference to character

        if (iscntrl(c)) {
            printf("%d\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
    }
    return 0;
}