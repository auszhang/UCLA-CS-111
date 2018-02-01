NAME: Christopher Aziz
EMAIL: caziz@ucla.edu

# Project 1B: Compressed Network Communication

## Included Files

File           | Details
-------------- | -------
lab1b-client.c | Source code for client which opens a connection to a server specified by the `--port` option. The client can then send input from the keyboard to the server (while echoing to the display), and send input from the server to the display.
lab1b-server.c | Source code for server which connects with the client, receives the client's commands, and sends the commands to the shell. The server then serves the client the outputs of those commands.
Makefile       | Makefile that includes the targets `build`, `check`, `clean`
README         | This file which describes each of the included files and other information about the submission

## Research

I used the sockets tutorial provided in the project description in order to understand how to make a TCP connection in C. This tutorial is available at: http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
I also referred to the Unit Socket series recommended by the TA at: https://www.tutorialspoint.com/unix_sockets/client_server_model.htm
