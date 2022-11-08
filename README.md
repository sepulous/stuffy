# Stuffy

Stuffy is a simple file archiver for UNIX systems that I threw together as a homework assignment for school.

# Building

Stuffy can be built by simply running `make`.

# Commands

Creating an archive/adding files to it:

`./stuffy -a <archive> <file>`

Removing file from archive:

`./stuffy -r <archive> <file>`

Listing files in archive:

`./stuffy -l <archive>`

Extracting file from archive:

`./stuffy -e <archive> <file>`

`./stuffy -e <archive> <file> > output`