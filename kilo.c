/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig {
  int screenrows;
  int screencols;
  struct termios original_termios;
};

struct editorConfig E;

/*** terminal ***/

void die(const char *s) {
  // clean up the screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  // print description of error in global errno variable
  perror(s);

  // signal failure
  exit(1);
}

// restore original terminal attributes
void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1)
    die("tcgetattr");
}

void enableRawMode() {
  // store original terminal attributes, to restore later
  if (tcgetattr(STDIN_FILENO, &E.original_termios) == -1) die("tcgetattr");

  // register a function to be executed when program exits
  atexit(disableRawMode);

  // copy attributes from original
  struct termios raw = E.original_termios;

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
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  }
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) -1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}

// common way to return multiple values: use multiple pointers
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  // ask IO Control for Terminal WINdow SiZe
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** append buffer ***/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** output ***/

void editorDrawRows(struct abuf *ab) {
  int y;

  for (y = 0; y < E.screenrows; y++) {
    abAppend(ab, "~", 1);

    if (y < E.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;

  // hide the cursor while the redraw happens
  abAppend(&ab, "\x1b[?25l", 6);

  // write escape sequence to append buffer
  // \x1b is the escape character, followed by [
  // J command erases the screen
  // 2 is argument to erase whole screen
  // 1 would clear screen up to cursor
  // 0 would clear screen from cursor to the end of the screen
  abAppend(&ab, "\x1b[2J", 4);

  // position cursor to top-left
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);

  abAppend(&ab, "\x1b[H", 3);

  // show the cursor again
  abAppend(&ab, "\x1b[?25h", 6);

  // write out buffer's contents to screen
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** init ***/

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
