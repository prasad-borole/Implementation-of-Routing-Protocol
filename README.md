AUTHOR:
-------
Prasad Borole(pborole@buffalo.edu)

INTRODUCTION:
------------
Filename: distanceVector.c
Compiler: GCC

This file contains Implementation of Routing Protocols (using C and UDP Sockets). 
A simplified version of the Distance Vector Routing protocol using Bellman-Ford algorithm is implemented. This program runs on the top of servers behaving as routers. The select function is used to carry out multiplexing between user commands and sending/reception of Distance Vectors.

USAGE:
----
Compile and run file:
root>gcc -o distanceVector distanceVector.c
root>./distanceVector -t <topology-file-name> -i <routing-update-interval>

Commands supported:
1.update <server-ID1> <server-ID2> <Link Cost>
  Updates link cost between two given servers. Supports "inf" keyword for denoting broken connection between 2 servers.
2.step
  Triggers routing update to neighbors
3.packets
  Display the number of distance vector packets this server has received since its last invocation.
4.display
  Display the current routing table
5.disable<server-ID>
  Disable the link to a given server.
6.crash
  Close all connections.
  
IMPLEMENTATION:
---------------
The program is designed mainly in 4 components:
1.	Data-structures Implementation
2.	Main program
3.	Bellman Ford Algorithm implementation
4.	Supporting functions

Each of these components are chained with each other component and together provides the Distance Routing Protocol Functionality.
The pborole_proj2.pdf file explains all these design implementations in detail.
  
