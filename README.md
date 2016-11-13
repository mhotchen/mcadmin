# mcadmin

Easy administration of memcache from the CLI. Lightweight and usable over SSH.

## Features

* View and delete items
* See stats on hits/sets/etc.
* See stats on network, memory and CPU usage
* View detailed slab information
* Invalidate all items

## Build

### Requirements

* A C compiler
* CMake
* Headers for ncurses and libcdk5 (`apt install ncurses-dev libcdk5-dev`)

### Installation

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
Process      PID: 1992, uptime: 7.9 hours
CPU time     user: 2.17, system: 3.96
Memory       used: 36.0 MB, available: 64.0 MB
Network      read: 163.3 MB, written: 2.0 MB
Connections  current: 5, total: 40032
Commands     set: 19996, get: 5h/8m, delete: 0h/0m, cas: 0h/0m/0b
             incr: 0h/0m, decr: 0h/0m, touch: 0h/0m
```

### Slab view

```
mcadmin | q: quit | f: flush all content | /: find | s: switch view
Slab 18 (3 of 4) | <TAB>: cycle through slabs
Chunks  amount: 2530, used: 2499, free: 31, size: 4.4 KB
Pages   amount: 11, chunks per page: 230
Memory  amount: 11.0 MB, used: 8.7 MB, free: 2.2 MB
```

