/* 
   LoL Editor source file.
   Majorly inherited from the kilo source code with an intent to learn,
   And to add new features on my own. 
   @nxck2005
*/

// Happy New Year!

/*** includes ***/

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>


/*** defines ***/

/* LoL version */

#define LOL_VERSION "0.0.1"

/* Control key masks off bit 5 and 6 from the character */

#define CTRL_KEY(k) ((k) & 0x1f)


/*** data ***/

// State of terminal originally

struct editorConfig {
    int cx, cy;
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
    
    /* 
       No change is made until all currently written data has been transmitted, 
       at which point any received but unread data is also discarded.
    */

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

/* Get current cursor position
   Reply is a escape sequence; documented as Cursor Position Report
   https://vt100.net/docs/vt100-ug/chapter3.html#CPR */


int getCursorPosition(int *rows, int *cols) {

    // Read response into a buffer
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        
        // Last character of response is R, hence
        if (buf[i] == 'R') break;
        i++;
    }
    // Append string terminator to last byte
    buf[i] = '\0';

    // Check for valid escape sequence
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;

    // Use sscanf to parse the two numbers in that escape sequence
    // And put it in rows and cols
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    return 0;
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

        /* If ioctl couldn't return size, 
           Move cursor to bottom right, and observe result/get cursor pos */

        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);

    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}


/*** append buffer ***/

/* 

   To do one big write so whole screen updates at once,
   replacing all write calls, with code that appends the string to a buffer,
   and then write() this buffer out at the end. C doesn't have dynamic strings,
   so creating our own dyn string type with one operation type: appending

*/

struct abuf {

    // Pointer to buffer in memory
    char *b;

    // Length
    int len;
};


// Constructor for abuf type
#define ABUF_INIT {NULL, 0}

// Append operation on the dynamic string type
void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    // if allocation fails, return
    if (new == NULL) return;

    // Copies new string at the end of first string in reallocated space
    memcpy(&new[ab->len], s, len);

    // Point to new space now
    ab->b = new;
    ab->len += len;
}

// Destructor
void abFree(struct abuf *ab) {
    free(ab->b);
}


/*** output ***/

void editorDrawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.screenrows; y++) {

        /* If cursor is at specific position, print welcome msg there */

        if (y == E.screenrows / 3) {

            /* welcome screen */

            /* buffer to hold formatted string */
            char welcome[80];

            /* 
                snprintf writes a formatted string with a null terminator, 
                into a char array. Write it to buffer for writing to abbuf
            */

            int welcomelen = snprintf(welcome, sizeof(welcome), 
            "LoL : Lots Of Lines Editor -- version %s", LOL_VERSION);

            /* Add padding to center the text */

            /* 
               To properly center a string, divide screen width by 2, 
               then subtract half of the string's length. 
               (cols/2) - (welcome/2) => (cols - welcome) / 2
               
               Tells us how far from the left edge of the screen to 
               start printing.
               Fill space with space characters, except first char is ~ 
            */

            int padding = (E.screencols - welcomelen) / 2;

            /* Executed once, add one tilde, and reduce padding by 1 as one space is used */
            if (padding) {
                abAppend(ab, "~", 1);
                padding--;
            }

            /* Fill remaining padding with spaces */
            while (padding--) abAppend(ab, " ", 1);

            /* if terminal is too small, truncate the message */

            if (welcomelen > E.screencols) welcomelen = E.screencols;

            /* Finally, append buffer to write dynamic buffer */

            abAppend(ab, welcome, welcomelen);

        } else {

            /* Print tildes for every other row */
            abAppend(ab, "~", 1);

        }

        /* 
         * K command: Erase In Line
         * 0 : default argument, erases part of line to the right of the cursor
         * Hence clears entire row
         */
        abAppend(ab, "\x1b[K", 3);

        if (y < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
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

    /* 
       buffer = ab 
       draw rows using that buffer, then destruct it at the end 
    */
    struct abuf ab = ABUF_INIT;

    // First, turn off cursor while repainting
    // h, l : Set and Reset for different terminal features or "modes"
    // argument ?25 : cursor hiding/showing

    abAppend(&ab, "\x1b[?25l", 6);

    // Escape sequence to position cursor at 1,1

    abAppend(&ab, "\x1b[H", 3);

    // Print tildes on rows then reposition at 1,1 again

    editorDrawRows(&ab);
    abAppend(&ab, "\x1b[H", 3);

    // Re-enable the cursor

    abAppend(&ab, "\x1b[?25h", 6);

    // Write all changes and destruct
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
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
    E.cx = 0;
    E.cy = 0;
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
