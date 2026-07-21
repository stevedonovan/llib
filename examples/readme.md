These are instructive little programs that use llib.

To build them, use the makefile like so:

$ make PROG=name

where name is the program name without extension.

For programs that use llib-p in addition (sorry, POSIX is not Windows!)
say:

$ make -f llibp.make P=test-rx

Go into ../llib-p and say 'make' first.

The `cexpr` program is a special snowflake that needs C2x features
like `auto` and GNU features like statement expressions, so use:

$ make -f cexpr.mak


