/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct termios original_termios;

/*** terminal ***/

void die(const char *s) {
  // print description of error in global errno variable
  perror(s);

  // signal failure
  exit(1);
}

// restore original terminal attributes
void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1)
    die("tcgetattr");
}

void enableRawMode() {
  // store original terminal attributes, to restore later
  if (tcgetattr(STDIN_FILENO, &original_termios) == -1) die("tcgetattr");

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
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
  int nread;
  char c;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }

  return c;
}

/*** input ***/

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
    editorProcessKeypress();
  }

  return 0;
}
