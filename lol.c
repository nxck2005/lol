#include <unistd.h>
#include <termios.h>
#include <stdio.h>

void enableRawMode() {
    struct termios raw;

    tcgetattr(STDERR_FILENO, &raw);

    raw.c_lflag &= ~(ECHO);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}