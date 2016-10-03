# mcadmin

An NCurses admin UI for memcache

The aim of the project is to allow for easier administration of your memcache instances using an NCurses UI which is available over SSH.

## Build

Requires ncurses-dev and libcdk5-dev headers (`apt install ncurses-dev libcdk5-dev`).

```
cmake .
make
```

## Run

```
./mcadmin localhost 11211
```


## Preview

### Primary view

```
mcadmin | q: quit | f: flush all content | /: find | s: switch view
Memcache 2.0.21-stable, PID: 6755, uptime: 1436
CPU time: 0.34 (usr), 0.94 (sys)
Memory: used: 39723448 bytes, available: 67108864 bytes
Network: read: 39293526 bytes, written: 4732 bytes
Connections: 5 (curr), 10006 (tot)
Commands: set: 9999, get: 0h/0m, delete 0h/0m, cas 0h/0m/0b
          incr: 0h/0m, decr 0h/0m, touch 0h/0m
```

### Slab view

```
mcadmin | q: quit | f: flush all content | /: find | s: switch view
Slab 12 (2 of 4) | <TAB>: cycle through slabs
Chunks: amount: 2655, used: 2500, free: 155, chunk size: 1184
Pages: amount: 3, chunks per page: 885
Memory: max: 3143520, used: 2370000, free: 773520
```

## Status

This is currently stable enough to use, but still in active development. Pull requests and issues are very welcome.
