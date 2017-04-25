# mcadmin

Easy administration of memcache from the CLI. Lightweight and usable over SSH.

## Features

* View and delete items
* See stats on hits/sets/etc.
* See stats on network, memory and CPU usage
* View detailed slab information
* Invalidate all items

## Install

If you're using linux on either a 32 bit or 64 bit computer then check out the
Releases tab. Otherwise you can build it yourself.

From the Releases tab you can download a binary for your system architecture
then simply mark it as executable and place wherever suits you best (eg.
/usr/bin)

## Build

### Requirements

* A C compiler
* CMake
* Headers for ncurses and libcdk5 (`apt install ncurses-dev libcdk5-dev`)

To install globally:

```
git clone https://github.com/mhotchen/mcadmin.git
cd mcadmin
cmake .
sudo make install
```

To create an executable binary at ./mcadmin:

```
git clone https://github.com/mhotchen/mcadmin.git
cd mcadmin
cmake .
make
```

## Run

```
mcadmin localhost 11211
```

## Preview

### Primary view

```
mcadmin | q: quit | f: flush all content | /: find | s: switch view
Memcache 2.0.21-stable
Process      PID: 1992, uptime: 8.0 hours
CPU time     user: 2.20, system: 3.98
Memory       total: 64.0 MB, used: 36.0 MB
Network      read: 163.3 MB, written: 2.2 MB
Connections  current: 5, total: 40036
Commands     set: 19996, get: 5h/8m, delete: 0h/0m, cas: 0h/0m/0b
             incr: 0h/0m, decr: 0h/0m, touch: 0h/0m
```

### Slab view

```
mcadmin | q: quit | f: flush all content | /: find | s: switch view
Slab 18 (3 of 4) | <TAB>: cycle through slabs
Chunks  total: 2530, used: 2499, size: 4.4 KB
Pages   total: 11, chunks per page: 230
Memory  total: 11.0 MB, used: 8.7 MB
```

