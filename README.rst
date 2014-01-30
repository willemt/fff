
FileWatcher
=====

What?
-----
C file watcher, designed to listen to filesystem events, BSD licensed

How does it work?
-----------------
As directories become deeper it becomes more challenging to keep listening to all filesystem events.

**Hot files**
Files that have had a lot of events are considered *hot*, and are watched more carefully

**Periodic optimistic checker**
Periodically the entire file system will be scanned for changes

**Proximity**
Files close to hot files are listened to more often

See filewatcher.h for full documentation.

Dependencies
------------
File watcher needs libuv.

Building
--------
$make

