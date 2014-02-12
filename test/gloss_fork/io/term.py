# Copyright 2011 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

r'''
TODO: register a signal handler for SIGWINCH to update sizes.


Standard control character bindings:
ctrl-d EOF:   End-of-file; commonly used to exit from a program or shell.
ctrl-h ERASE: backspace.
ctrl-u KILL:  Deletes text from cursor back to start of line.
ctrl-\ QUIT:  SIGQUIT.
ctrl-z SUSP:  Suspend; pauses the current job and puts it in the background (see "unix job control").
ctrl-c INTR:  Interrupt; interrupt the job currently running in the foreground (see "unix SIGINT").


notes from 'man termios':

In canonical mode input processing, terminal input is processed in units of lines. A line is delimited by a line feed '\n' character, an end-of-file (EOF) character, or an end-of-line (EOL) character. A read request will not return until an entire line has been typed, or a signal has been received. No matter how many bytes are requested in the read call, at most one line is returned. It is not, however, necessary to read a whole line at once.

{MAX_CANON} is a limit on the number of bytes in a line. The behavior of the system when this limit is exceeded is the same as when the input queue limit {MAX_INPUT}, is exceeded.

Erase and kill processing occur when an ERASE or KILL character is received. This processing affects data in the input queue that has not yet been delimited by a newline NL, EOF, or EOL character (the current line). ERASE deletes the last character in the current line; KILL deletes all data in the current line. Neither has any effect if there is no data in the current line. These characters are then omitted from the queue.


In noncanonical mode input processing, input bytes are not assembled into lines, and erase and kill processing does not occur. The values of the MIN and TIME members of the c_cc array are used to determine how to process the bytes received.

MIN represents the minimum number of bytes that should be received when the read function successfully returns. TIME is a timer of 0.1 second granularity (TIME=10 for 1 second) that is used to time-out bursty and short term data transmissions. If MIN is greater than { MAX_INPUT}, the response to the request is undefined. The four possible values for MIN and TIME and their interactions are described below.

Case A: MIN > 0, TIME > 0
TIME serves as an inter-byte timer and is activated after the first byte is received. Since it is an inter-byte timer, it is reset after a byte is received. The interaction between MIN and TIME is as follows: as soon as one byte is received, the inter-byte timer is started. If MIN bytes are received before the inter-byte timer expires (remember that the timer is reset upon receipt of each byte), the read is satisfied. If the timer expires before MIN bytes are received, the characters received to that point are returned to the user. Note that if TIME expires at least one byte is returned because the timer would not have been enabled unless a byte was received. In this case the read blocks until the MIN and TIME mechanisms are activated by the receipt of the first byte, or a signal is received. If data is in the buffer at the time of the read(), the result is as if data had been received immediately after the read().

Case B: MIN > 0, TIME = 0
No timer. A pending read blocks until MIN bytes are received, or a signal is received. The read may block indefinitely.

Case C: MIN = 0, TIME > 0:
Since MIN = 0, TIME no longer represents an inter-byte timer; It now serves as a read timer that is activated as soon as the read function is processed. A read is satisfied as soon as a single byte is received or the read timer expires. If the timer expires, no bytes are returned. The read will not block indefinitely. If data is in the buffer at the time of the read, the timer is started as if data had been received immediately after the read.

Case D: MIN = 0, TIME = 0
The minimum of either the number of bytes requested or the number of bytes currently available is returned without waiting for more bytes to be input. If no characters are available, read returns a value of zero, having read no data.


Writing Data and Output Processing
When a process writes one or more bytes to a terminal device file, they are processed according to the c_oflag field (see the Output Modes section). The implementation may provide a buffering mechanism; as such, when a call to write() completes, all of the bytes written have been scheduled for transmission to the device, but the transmission will not necessarily have been completed.


Special Characters:

INTR, QUIT, SUSP: recognized if ISIG is enabled. Generate a SIGINT/SIGQUIT/SIGTSTP signal which is sent to all processes in the foreground process group for which the terminal is the controlling terminal. Discarded when processed.

ERASE, KILL: recognized if ICANON is set. Deletes one/all characters in line buffer. Discarded when processed.

EOF: recognized if ICANON is set. All the bytes waiting to be read are immediately passed to the process; the EOF is discarded. Thus, if there are no bytes waiting (the EOF occurred at the beginning of a line), a byte count of zero is returned from the read(), representing an end-of-file indication.

NL, EOL: recognized if ICANON is set. NL is '\n', EOL is an alternate.

CR: recognized if ICANON is set. It is '\r'. When ICANON and ICRNL are set and IGNCR is not set, this character is translated into a NL, and has the same effect as a NL character.

STOP, START: recognized if IXON (output control) or IXOFF (input control) is set. Can be used to temporarily suspend/resume output. It is useful with fast terminals to prevent output from disappearing before it can be read.


The following special characters are extensions defined by this system and are not a part of 1003.1 termios.

EOL2: Secondary EOL character. Same function as EOL.

WERASE: Special character on input and is recognized if the ICANON flag is set. Erases the last word in the current line according to one of two algorithms. If the ALTWERASE flag is not set, first any preceding whitespace is erased, and then the maximal sequence of non-whitespace characters. If ALTWERASE is set, first any preceding whitespace is erased, and then the maximal sequence of alphabetic/underscores or non alphabetic/underscores. As a special case in this second algorithm, the first previous non-whitespace character is skipped in determining whether the preceding word is a sequence of alphabetic/undercores. This sounds confusing but turns out to be quite practical.

REPRINT: Special character on input and is recognized if the ICANON flag is set. Causes the current input edit line to be retyped.

DSUSP: Has similar actions to the SUSP character, except that the SIGTSTP signal is delivered when one of the processes in the foreground process group issues a read() to the controlling terminal.

LNEXT: recognized if the IEXTEN is set. Causes the next character to be taken literally.

DISCARD: recognized if the IEXTEN is set. Toggles the flushing of terminal output.

STATUS: recognized if the ICANON is set. Causes a SIGINFO signal to be sent to the foreground process group of the terminal. Also, unless the NOKERNINFO flag is set, it causes the kernel to write a status message to the terminal that displays the current load average, the name of the command in the foreground, its process ID, the symbolic wait channel, the number of user and system seconds used, the percentage of cpu the process is getting, and the resident set size of the process.


Local Modes
Values of the c_lflag field is composed of the following bit masks:

ECHOKE      visual erase for line kill
ECHOE       visually erase chars
ECHO        enable echoing
ECHONL      echo NL even if ECHO is off
ECHOPRT     visual erase mode for hardcopy
ECHOCTL     echo control chars as ^(Char)
ISIG        enable signals INTR, QUIT, SUSP
ICANON      canonicalize input lines
ALTWERASE   use alternate WERASE algorithm
IEXTEN      enable DISCARD and LNEXT
EXTPROC     external processing
TOSTOP      stop background jobs from output
FLUSHO      output being flushed (state)
NOKERNINFO  no kernel output from VSTATUS
PENDIN      XXX retype pending input (state)
NOFLSH      don't flush after interrupt

If ECHO is set, input characters are echoed back to the terminal.

if ICANON is set:

  If ECHOE is set, ERASE causes the terminal to erase the last character in the current line from the display, if possible.

  If ECHOK is set, the KILL character causes the current line to be discarded and the system echoes the '\n' character after the KILL character.

  If ECHOKE is set, the KILL character causes the current line to be discarded and the system causes the terminal to erase the line from the display.

  If ECHOPRT is set, the system assumes that the display is a printing device and prints a backslash and the erased characters when processing ERASE characters, followed by a forward slash.

  If ECHOCTL is set, the system echoes control characters in a visible fashion using a caret followed by the control character.

  If ALTWERASE is set, the system uses an alternative algorithm for determining what constitutes a word when processing WERASE characters (see WERASE).

  If ECHONL is set, the '\n' character echoes even if ECHO is not set.


If ISIG is set, check for INTR, QUIT, and SUSP.

If NOFLSH is set, the normal flush of the input and output queues associated with the INTR, QUIT, and SUSP characters are not be done.

If IEXTEN is set, implementation-defined functions are recognized from the input data. How IEXTEN being set interacts with ICANON, ISIG, IXON, or IXOFF is implementation defined.

If ICANON is set, an upper case character is preserved on input if prefixed by a \ character. In addition, this prefix is added to upper case characters on output.

In addition, the following special character translations are in effect:

for:    use:
`       \'
|       \!
~       \^
{       \(
}       \)
\       \\

If TOSTOP is set, the signal SIGTTOU is sent to the process group of a process that tries to write to its controlling terminal if it is not in the foreground process group for that terminal. This signal, by default, stops the members of the
process group. Otherwise, the output generated by that process is output to the current output stream. Processes that are blocking or ignoring SIGTTOU signals are excepted and allowed to produce output and the SIGTTOU signal is not sent.

If NOKERNINFO is set, the kernel does not produce a status message when processing STATUS characters (see STATUS).

Special Control Characters
The special control characters values are defined by the array c_cc. This table lists the array index, the corresponding special character, and the system default value. For an accurate list of the system defaults, consult the header file

from OS X <ttydefaults.h>:

#define CTRL(x) (x&037)

#define CEOF      CTRL('d')
#define CEOL      0xff      // XXX avoid _POSIX_VDISABLE
#define CERASE    0177
#define CINTR     CTRL('c')
#define CSTATUS   CTRL('t')
#define CKILL     CTRL('u')
#define CMIN      1
#define CQUIT     034       // S, ^\ 
#define CSUSP     CTRL('z')
#define CTIME     0
#define CDSUSP    CTRL('y')
#define CSTART    CTRL('q')
#define CSTOP     CTRL('s')
#define CLNEXT    CTRL('v')
#define CDISCARD  CTRL('o')
#define CWERASE   CTRL('w')
#define CREPRINT  CTRL('r')
#define CEOT      CEOF
'''


import sys as _sys
import struct as _struct
import fcntl as _fcntl
import termios as _tio
import tty as _tty
import copy as _copy
import gloss_fork.meta as _meta


_meta.alias_list([
  ('setraw',    'set_raw'),
  ('setcbreak', 'set_cbreak'),
], src_module=_tty)


def window_size(f=_sys.stdout):
  '''
  TODO: replace with shutil.get_terminal_size()?
  '''
  if not f.isatty():
    return (128, 0)
  try:
    cr = _struct.unpack('hh', _fcntl.ioctl(f, _tio.TIOCGWINSZ, 'xxxx')) # arg string length indicates length of return bytes
  except:
    print('gloss.term.window_size: ioctl failed', file=_sys.stderr)
    raise
  return int(cr[1]), int(cr[0])


# Indexes for termios list (see <termios.h>).
IFLAG = 0
OFLAG = 1
CFLAG = 2
LFLAG = 3
ISPEED = 4
OSPEED = 5
CC = 6

RAW, CBREAK, SILENT = (object() for i in range(3)) # terminal modes

when_vals = (_tio.TCSANOW, _tio.TCSAFLUSH, _tio.TCSADRAIN)

class set_mode():
  '''
  context manager for altering terminal modes.
  if no file descriptor is provided, it defaults to stdin.
  '''
  
  def __init__(self, mode, fd=None, when=_tio.TCSAFLUSH, min_bytes=1, delay=0):
    assert when in when_vals
    if fd is None:
      fd = _sys.stdin.fileno()
    self.fd = fd
    self.when = when
    self.original_attrs = _tio.tcgetattr(fd)
    a = _copy.deepcopy(self.original_attrs)
    if delay > 0:
      vtime = int(delay * 10)
      assert vtime > 0
    else:
      vtime = 0
    if mode is RAW:
      a[IFLAG] &= ~(_tio.BRKINT | _tio.ICRNL | _tio.INPCK | _tio.ISTRIP | _tio.IXON)
      a[OFLAG] &= ~(_tio.OPOST)
      a[CFLAG] &= ~(_tio.CSIZE | _tio.PARENB)
      a[CFLAG] |= _tio.CS8
      a[LFLAG] &= ~(_tio.ECHO | _tio.ICANON | _tio.IEXTEN | _tio.ISIG)
      a[CC][_tio.VMIN] = min_bytes
      a[CC][_tio.VTIME] = vtime
    elif mode is CBREAK:
      a[LFLAG] &= ~(_tio.ECHO | _tio.ICANON)
      a[CC][_tio.VMIN] = min_bytes
      a[CC][_tio.VTIME] = vtime
    elif mode is SILENT:
      a[LFLAG] &= ~(_tio.ECHO)
    else:
      raise ValueError('unkown mode for term.set_mode: {}'.format(mode))
    self.attrs = a

  def __enter__(self):
    _tio.tcsetattr(self.fd, self.when, self.attrs)

  def __exit__(self, exc_type, exc_val, exc_tb):
    _tio.tcsetattr(self.fd, self.when, self.original_attrs)

