#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios original_termios;

// restore original terminal attributes
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enableRawMode() {
  // store original terminal attributes, to restore later
  tcgetattr(STDIN_FILENO, &original_termios);

  // register a function to be executed when program exist
  atexit(disableRawMode);

  // copy attributes from original
  struct termios raw = original_termios;

  // disable ctrl-s, ctrl-q
  raw.c_iflag &= ~(ICRNL | IXON);

  // disable more flags part of "raw mode"
  raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP);

  // character size to 8 bits per byte (probably already set)
  raw.c_cflag |= ~(CS8);

  // disable the output translation of "\n" into "\r\n"
  raw.c_oflag &= ~(OPOST);

  // lflags is local flags
  // other flags include input flags (c_iflag), output flags (c_oflag) and
  // control flags (c_cflag)
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  // minimum bytes of input to read
  raw.c_cc[VMIN] = 0;
  // timeout read after 1 * 100ms
  raw.c_cc[VTIME] = 1;

  // write terminal attributes
  // TCSAFLUSH waits for all output to be written to terminal before applying change
  // also discards anything not yet read
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  enableRawMode();

  while (1) {
    char c = '\0';

    read(STDIN_FILENO, &c, 1);

    // is control character
    if (iscntrl(c)) {
      printf("%d\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }

    if (c == 'q') break;
  }

  return 0;
}
