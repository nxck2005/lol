/*** includes ***/

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


/*** data ***/

// State of terminal originally
struct termios original_termios;


/*** terminal ***/

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

    // Disable Ctrl S and Ctrl Q (software flow control) : IXON
    // and carriage return to make sure Ctrl M is read properly, and enter too
    // (ICRNL)
    // Rest are misc flags; by tradition
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    // Turn off all output postprocessing, like adding of /r after /n automatically
    raw.c_oflag &= ~(OPOST);

    // Turn off echo, canonical mode, Ctrl V, and sigterms
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // Misc flag; turns character size to 8 bytes
    // Although most systems should already have this
    raw.c_cflag |= (CS8);

    /* Indexes into control character [cc] field */
    /* VMIN sets min number of bytes of input needed before read() can return */
    /* VTIME is max amount of time to wait before read() returns (in 10ths of sec)*/
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 10;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


/*** init ***/

int main() {
    enableRawMode();
    
    while (1) {
        char c = '\0';
        read(STDIN_FILENO, &c, 1);
        
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