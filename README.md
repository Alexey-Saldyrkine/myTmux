# myTmux
My own implementation of tmux.

Turns a terminal into a 'desktop' like environment, where you can create sub terminals, that you can move, resize and layer one over another.

Uses ncurses to control terminal display.

### How it works
The application creates a pty (pseudoterminal) using linux system calls for each subterminal. The slave side of the pty runs on a separate thread, while the master side is used by the main terminal. When, in the terminal, a subterminal is selected all key presses are passed the salve side of that's subterminal pty. The pty acts exactly the same as any other terminal, as all modern terminals are pty. The salve pty accepts the key presses, as if it was not a subterminal, meaning anything you can do in a terminal you can do in a subterminal. The master pty reads the output of the salve pty and translates it to the subwindow. The translation processes most [ANSI codes](https://man7.org/linux/man-pages/man4/console_codes.4.html) into ncurses calls on the sub window. This includes cursor movement, screen modifiers, color, operating system control codes.

#examples:
