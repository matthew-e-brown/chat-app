# Client-Server Chat Application

Rather than calling this folder `assignment1`, `assignment2`, or something of
the sort, this folder is just called `chat-app`. This is because the same
project is used for all of the assignments of COIS-4310, and it is simply built
upon. This repository will have tagged commits for the points `chat-app` was in
when each Assignment was handed in.

Documentation is very important to the professor, and this README will actually
be handed in alongside the code at every due date. As such, it *may* have some
essential comments.

**I will be putting my references for each "assignment" in this document.**


## Assignment 1

The first assignment was to implement the basic app and get it up and running.
The "theory" part of the assignment was to implement an official(-ish) RFC
document to explain how the program's messaging system works.


### References

Many a help article was used to help complete this first assignment. The main
ones were:

- [Beej's Guide to Network Programming][beej];
- [epoll(7) &mdash; Linux manual page][epoll-man];
- These guides on NCURSES:
  - [NCURSES Programming HOWTO][ncurses-1],
  - [Ncurses Programming Guide][ncurses-2], and
  - [Programming in color with ncurses][ncurses-3]; and
- Countless Stack Overflow pages. Unfortunately, since I visited 263 different
  Stack Overflow URLs (according to my history trends analyzer extension in
  Chrome), in the last two weeks, nearly all of which pertain to this
  assignment, I cannot possibly hope to list all of them. However, if requested
  by the professor or marker, I will provide a tab-separated file of my Stack
  Overflow history during this period.


[ncurses-1]: https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/index.html
[ncurses-2]: http://www.cs.ukzn.ac.za/~hughm/os/notes/ncurses.html
[ncurses-3]: https://www.linuxjournal.com/content/programming-color-ncurses
[beej]: https://beej.us/guide/bgnet/html/
[epoll-man]: https://man7.org/linux/man-pages/man7/epoll.7.html