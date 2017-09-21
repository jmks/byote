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

  // lflags is local flags
  // other flags include input flags (c_iflag), output flags (c_oflag) and
  // control flags (c_cflag)
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  // write terminal attributes
  // TCSAFLUSH waits for all output to be written to terminal before applying change
  // also discards anything not yet read
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  enableRawMode();

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    // is control character
    if (iscntrl(c)) {
      printf("%d\n", c);
    } else {
      printf("%d ('%c')\n", c, c);
    }
  }

  return 0;
}
