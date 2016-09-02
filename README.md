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

```
mcadmin | q: quit | f: flush all content | /: find | s: view slabs
Memcache 2.0.21-stable, PID: 3746, uptime: 31473
CPU time: 0.96 (usr), 1.64 (sys)
Memory: used: 52919862 bytes, available: 67108864 bytes
Network: read: 52492358 bytes, written: 400833 bytes
Connections: 5 (curr), 10025 (tot)
Commands: set: 9999, get: 1h/1m, delete 0h/0m, cas 0h/0m/0b
          incr: 0h/0m, decr 0h/0m, touch 0h/0m
```

## Status

This is currently in the early phases of development so please hang tight (or give me a hand).
