/*** includes ***/

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>


/*** defines ***/

/* Control key masks off bit 5 and 6 from the character */
#define CTRL_KEY(k) ((k) & 0x1f)


/*** data ***/

// State of terminal originally
struct termios original_termios;



/*** terminal ***/

void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1) die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    /* ECHO : bitflag/field 
       0000000000000000000000000000100  */

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

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}


// Keypress reading at the lowest level
char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}


/*** output ***/

void editorRefreshScreen() {

    /* 
     * Write escape sequence for clearing the screen to the terminal.
     * Esc sequences: 27 + [ + argument + command
     * 27 is decimal for \x1b, called escape character
     * command : J (Erase In Display) (VT100 sequences) 
     * argument: 2 (clear the entire screen) 
     */

    write(STDOUT_FILENO, "\x1b[2j", 4);
}



/*** input ***/

// Map keypresses to input operations
void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            exit(0);
            break;
    }
}


/*** init ***/

int main() {
    enableRawMode();
    
    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
