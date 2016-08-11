# mcadmin
An NCurses admin UI for memcache

The aim of the project is to allow for easier administration of your memcache instances using an NCurses UI which is available over SSH.

## Build

Requires ncurses-dev headers (`apt install ncurses-dev`).

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
┌─────────────────────────────────────────────────┐┌─────────────────────────────────────────────────┐
│Press q to quit                                  ││PID: 12670                                       │
│                                                 ││Version: 2.0.21-stable                           │
│                                                 ││Uptime: 8966 secs                                │
│                                                 ││Usage: 2.10 (usr), 1.66 (sys)                    │
│                                                 ││Connections: 5 (curr), 64 (tot)                  │
│                                                 ││Bytes: 76388 (read), 14942716 (writ)             │
│                                                 ││       0 (used), 67108864 (max)                  │
│                                                 ││Hit/miss:                                        │
│                                                 ││  get    0h/8m                                   │
│                                                 ││  delete 0h/1m                                   │
│                                                 ││  cas    0h/0m/0b                                │
│                                                 ││  incr   0h/0m                                   │
│                                                 ││  decr   0h/0m                                   │
│                                                 ││  touch  0h/0m                                   │
│                                                 ││                                                 │
└─────────────────────────────────────────────────┘└─────────────────────────────────────────────────┘

```

## Status

This is currently in the early phases of development so please hang tight (or give me a hand).
