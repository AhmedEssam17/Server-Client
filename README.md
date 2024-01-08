# Multi-Client Communication System
This C-based system enables multiple clients to communicate through a server using TCP/IP sockets. The server manages client connections, allows message exchange between clients, and logs messages into an SQLite database. Clients connect via unique IDs, sending and receiving messages by specifying the receiver's ID and message content in a specific format. The project includes server and client implementations using Winsock2 for networking and SQLite3 for database operations.

#### Running the Project

##### Server
Compile the server:
```bash
gcc -o server.exe server.c -lws2_32 -I. sqlite3.c
```

##### Client
Compile the client:
```bash
gcc -o client.exe client.c -lws2_32
```
