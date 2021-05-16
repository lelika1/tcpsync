## TCPSync

`TCPSync` consists of two C programs - `client` and `server`. Both are using `libpthread` and Linux TCP sockets. 

`client` starts 64 threads and generates lots of 8kbyte messages to the `server` over network. It is `server`'s responsibility to persist data it receives in per-client output file, keeping the order of incomming messages. 

`server` also gives a guarantee that when it explicitly `ack`s a message it is safely persisted on disk.

The goal of this project was to optimise throughput of the system and `client`'s run time.

## How to run the program

```sh
make run
```

The server stores data in the current working directory from which you run the program.

NOTE: There might be quite a lot of data, so consider adjusting the amount of messages to be sent by the client - `MESSAGES_COUNT` in `client.c`.