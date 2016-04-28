# RPMI
Redundant Message Passing Interface

An honors project for University of Illinois Urbana-Champaign CS241 Spring 2016

Ultimately the goal of this project is to implement a distributed RAID system but the current functionality is limited to small (<256 byte) messages.

### Version: 0.0005
### TODO:
* Establish working read/writes between client/controller/drives
* Configure log of tracked directories and files that can be mmap'd by disks under a tree structure
* Code cleanup/refactor
* Checkpoint system for rollback after drive failure
