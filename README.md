# Client-ThreadedServer
This project demonstrates a client and threaded server with file I/O

The server program runs as a daemon. It takes one command line parameter, 
which is assigned to the port number. The server accepts product records
from the client using a socket connection, processes the records, and then 
sends them back to the client using a different connection. Queues are used
to pass the records from station to station and to the writer thread, and then
finally back to the client.
Upon accepting a connection, the server creates a thread, which in turn 
creates five more threads, which are used to do various processing such as 
tax and total order cost. Upon receipt of a record with an id number 
of -1, the processes exit, displaying their individual reports. 

The client program attempts to establish a socket connection
with a running server program. The client opens a file, extracts
records, and sends them one at a time for processing to the server. A thread 
is created to establish another socket. The program sends the first record 
to the server with port and host name info for the second socket connection.
The sceond socket is used to receive records processed by the server and 
place them in an output file. This program accepts 5 command line parameters: 
input file, output file, server name, and server port number. 
