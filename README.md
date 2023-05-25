# Chat-Servers

## Overview of the project

A replicated chat server that uses User Datagram Protocol (UDP) to multicast chat messages

The figure below shows a sketch of the system we built. There are two kinds of processes: servers (represented by orange squares) and clients (represented by yellow circles). Each client is connected to one of the servers, and each server is connected to all the
other servers. The clients and servers can all run on different machines, or (e.g., for testing) they can all run on the same machine, e.g., in your VM. The system will provide different chat rooms. Client can join one of the chat rooms and then post messages, which should be delivered to all the other clients that are
currently in the same chat room. If there was only a single server, this would be easy: the server would simply send each message it receives to all the clients who are currently in the relevant room. However,
since there can be multiple servers, this simple approach does not work. Instead, the servers will internally use multicast to deliver the messages.


To make this a little more concrete, consider the following example. In the figure, client D is connected to
server S3. Suppose D joins chat room #3, which already contains A and B, and posts a message there (say,
"Hello"). S3 then multicasts D's message to multicast group #3, so it will be delivered to the other servers.
(All servers are members of all multicast groups.) The servers then forward D's message to all of the clients
that are currently in chat room #3. In this case, S3 will echo it back to D, and S1 will send it to A and B. S2
does not currently have any clients that are in chat room #3, so it ignores the message.


![Screenshot](https://github.com/saurabhraut99/Chat-Servers/blob/main/Chat%20server%20architecture.png)


The servers support three different ordering modes:
- Unordered multicast: Each server delivers the messages to its clients in whatever order the server
happens to receive it;
- FIFO multicast: The messages from a given client C should be delivered to the other clients in the
order they were sent by C; and
- Totally ordered multicast: The messages in a given chat room should be delivered to all the clients
in that room in the same order.
