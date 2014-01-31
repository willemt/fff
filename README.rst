
Friendlier File Follower
=====

What?
-----
fff is a file alteration monitor that listens to as many files as possible without exhausting OS watchers.

How does it work?
-----------------
fff periodically scans directories for changes. It adds watchers if it believes the directory has enough points.

Directories get points in different ways:
- 1 point for each file they contain (ie. contents point)
- 1 point when a file is modified (ie. event point)
- Their points get doubled if they aren't watched and a file is modified (ie. event points)

When a directory is modified:
- If we have free space on the watch-list, we add the directory to the watch-list
- Otherwise, if the modified directory passess the anti-thrashing test, the least-recently-modified watched directory is dropped from the watch-list. The modified directory takes it's place. The directory dropped from the watch-list has it's event based points set to zero.

**Anti-thrashing**
For a directory to be added to the watch-list it needs to have more points than the a directory with the least points on the watch-list.

See fff.h for full documentation.

Dependencies
------------
File watcher needs libuv.

Building
--------
$make

