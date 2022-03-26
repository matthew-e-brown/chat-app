# Client-Server Chat Application

## Preamble

This repo holds the code for my main project throughout most of COIS-4310H
(Computer Networking) at Trent University. It has been split off from my main
COIS-4310H repository so that I can have it public and show it off.

The code in here is not perfect. **There are memory leaks.** When you're reading
through it, keep in mind that it was done for class, and that the purpose of
that class was to learn how networking (HTTP, TCP, UDP, sockets, etc.)
works&mdash;becoming a C-master was not the focus, and this class was only the
second time I had used C.

I won't be making updates for correctness or bug fixes because this is meant to
show my schoolwork. If I do ever make any updates, they'll be in another branch.


## Introduction

Because this single project was used for assignments one, two, and four, I have
opted to keep them all in a single directory and simply tag the points at which
they were handed in. That way, I don't run into a situation where I have
`assignment1` with almost exactly the same files as `assignment2`.

The "RFCs" for each of the versions of the assignment will however be kept, so
as to emulate the sort of backwards compatibility found in real RFCs.
Because of this, rather than adopting SEMVER, each RFC simply completely
obsoletes the previous one&mdash;no matter how small the change.

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


## Assignment 2

The second assignment was to take the first assignment and make it use a more
official messaging protocol. More specifically, it was to make it so that the
program would recover on its own from packet loss, and to validate incoming
packets. In the case of this implementation, it was done using the built-in
`SHA1` function.

In order to test this packet-loss-recovery functionality, compile the code using
the `-D__DEBUG__` GCC flag. Alternatively, run `make d-server.o` or `make
d-both`. This will create a debug version of the server that corrupts packets 8%
of the time.


### References

Even though this program was entirely re-written between assignments, there
weren't that many extra articles used. Aside from an extra curses page or two
and the manual page for `SHA1`.

- [Curses "erase" vs "clear" question][ncurses-4]
- [sha1(3) &mdash; Linux manual page][sha1-man]
- [Fix for `EINTR` when resizing terminal screen][eintr]


## Assignment 3

The third assignment was unrelated.


## Assignment 4

The fourth and final assignment was to create a rudimentary form of "encryption"
for our clients to use to hide the plain-text of the messages as they pass over
the wire and through the server.

Of course, since this is a small school assignment, implementing real encryption
would be far out of scope. So, instead, we were instructed to use a simple
cipher. The may mine works is detailed in
[`client/encoding.c`](client/encoding.c).

If (and **only** if) the server is compiled with the `__DEBUG__` flag set (`make
d-server` or `make d-both`), it will print out the bytes of messages as they are
sent, to show that it cannot see the plain-text of the clients' messages.


### References

For this assignment, the only piece of code that was used from outside was John
Regehr's left- and right-rotate functions. They come from [his blog][rotate].



[ncurses-1]: https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/index.html
[ncurses-2]: http://www.cs.ukzn.ac.za/~hughm/os/notes/ncurses.html
[ncurses-3]: https://www.linuxjournal.com/content/programming-color-ncurses
[beej]: https://beej.us/guide/bgnet/html/
[epoll-man]: https://man7.org/linux/man-pages/man7/epoll.7.html

[ncurses-4]: https://lists.gnu.org/archive/html/bug-ncurses/2014-01/msg00007.html
[sha1-man]: https://linux.die.net/man/3/sha1
[eintr]: https://stackoverflow.com/a/6870391/10549827

[rotate]: https://blog.regehr.org/archives/1063