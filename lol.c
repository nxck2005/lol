// Happy New Year!

/*** includes ***/

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>


/*** defines ***/

/* Control key masks off bit 5 and 6 from the character */

#define CTRL_KEY(k) ((k) & 0x1f)


/*** data ***/

// State of terminal originally

struct editorConfig {
    int screenrows;
    int screencols;
    struct termios originaltermios;
};

struct editorConfig E;



/*** terminal ***/

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.originaltermios) == -1) die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.originaltermios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    /* ECHO : bitflag/field 
       0000000000000000000000000000100  */

    // Create copy of original terminal flags

    struct termios raw = E.originaltermios;

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


// Place number of rows and columns into given arguments
// Return value indicates degree of success

int getWindowSize(int *rows, int *cols) {

    // Has attributes like ws_col etc.

    struct winsize ws;
    
    // Sets ws's col and row values
    // By calling ioctl with the terminal as reference, and tio.. whatever
    // as request, and setting it to ws

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}


/*** output ***/

void editorDrawRows() {
    int y;
    for (y = 0; y < 24; y++) {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen() {

    /* 
     * Write escape sequence for clearing the screen to the terminal.
     * Esc sequences: 27 + [ + argument + command
     * 27 is decimal for \x1b, called escape character
     * command : J (Erase In Display) (VT100 sequences) 
     * argument: 2 (clear the entire screen) 
     */

    write(STDOUT_FILENO, "\x1b[2J", 4);

    // Escape sequence to position cursor at 1,1

    write(STDOUT_FILENO, "\x1b[H", 3);

    // Print tildes on rows then reposition at 1,1 again

    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);

}


/*** input ***/

// Map keypresses to input operations

void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}


/*** init ***/


/* Initialize all the fields in the terminal struct, E */

void initEditor() {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
    enableRawMode();
    initEditor();
    
    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
