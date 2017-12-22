Goal
UDP Client/Server
Setting up the exchange between the client and server in a secure way despite the lack of a formal connection (as in TCP) between the two, so that ‘outsider’ UDP datagrams (broadcast, multicast, unicast - fortuitously or maliciously) cannot intrude on the communication.
Introducing application-layer protocol data-transmission reliability, flow control and congestion control in the client and server using TCP-like ARQ sliding window mechanisms.

The congestion control section may have you modify what you made in the first part of the assignment and it is best to keep it in the back of your mind while proceeding. Try to write the client and server code as an independent units from the UDP reliability.

Required Reading
Chapter 8, 14, 22
Chapter 17: ioctl()
Section 20.5, Chapter 20: ‘Race’ Conditions

Client
bin/fclient- A file transfer client that can select a file from the server’s present working directory and receive the file over a UDP connection.
Arguments
Since the arguments for this program are numerous you should read them from a file named client.in. The values are each on their own line and will always be in this order:
Server IP address (not hostname)
Server Port
A seed for srand() (more at the end of behavior section) 
Probability of datagram loss [0.0, 1.0] 0 means no loss, 1 means total loss
Maximum receiving sliding-window in datagrams (for reliability)
μ - Mean ms read delay (for reliability)
Client Commands
From a prompt accept the following commands:
list - lists the files available in the server’s public directory.
download FILE [> FILENAME] - requests that the server transmit one of the available files. The file being downloaded should be displayed on stdout unless the redirection symbol and file name are specified.
quit - Quits the client gracefully
Client Behavior
Call srand(3) with the seed value from client.in
The client should create a UDP socket and bind on one IP that is a unicast address and that is linked to the network (e.g, not localhost) and port 0. This will give you an ephemeral port assigned randomly by the kernel. 
Use getsockname(2) to print in a readable format the ip and port the socket is now bound to. 
Next, connect the client to the server’s IP and port.
Use getpeername(2) to print in a readable format the server port and ip to confirm the proper connection.
Prompt to the user for a valid command. If an invalid command, do nothing and reprompt.
When the user types list or download the client sends a request datagram instructing the server to respond accordingly.  (Note: you’ll need a timeout in case the datagram is lost, start with 5 seconds)
In the case of a timeout you should retransmit the request and hope for the best. 
For debugging purposes, your client should give up, print out that it couldn’t get a response and exit after the 5th attempt.
The server will reply with a connection datagram (more on this in the server section) containing the new port number that the file will be sent over. This also serves as acknowledgement for the command sent. Using this connection datagram the client should reconnect its socket to this new connection server port. Upon success, send an ‘acknowledgements’ datagram to the server. 
In the event of the server timing out with when setting up a  connection,  it should retransmit two copies of its ‘ephemeral port number’ message, one on its ‘listening’ socket and the other on its ‘connection’ socket (why?).
All communication between the server and client is now performed on this connection.
The client now receives datagrams from the server from the new socket. 
For list, print out the file list in a readable format (if you format your datagram contents nicely, you can just dump the contents as they come in).
For download, as the datagrams arrive print out the  contents as they come in, in order, with nothing missing and with no duplication of content, directly on to stdout.
Added 10/24, NOTE: The client should use a separate thread to periodically read from the receive buffer and print the data to stdout. See the reliability section #5  for more details.
Additional Notes
Using rand(3) and the probability specified on the client.in to simulate transmission loss. 
All client datagrams are sent to the server.
All incoming datagrams from the server & all client  ‘acknowledgements’ are dropped by the client (simulating transmission loss)  using 
(rand() / (double)RAND_MAX) < probability
Each time you receive a datagram you should be reading the data into a buffer. This buffer should be limited not in bytes but by datagram unit as specified in client.in. If the buffer is full the data is dropped. (More on this in UDP Congestion & Reliability) 
Server
bin/fserver - UDP based file transfer server that reports the files available and delivers them over UDP datagrams upon request.
Arguments
The arguments for this program are not numerous but to stay consistent you should read them in the same fashion as the client. The file will be named server.in. The values are each on their own line and will always be in this order:
Port number for server to listen on.
Maximum sending sliding-window size.
Path to files to list on server
Server Behavior
Open a UDP socket and bind on all IP that is a unicast address and linked to the network as well as localhost and the port found in server.in.
 Print in a readable format all ip and subnet address of each socket you bind to
Use your favorite I/O multiplexing function to listen on these sockets.
Upon receiving a datagram (use recvfrom() or recvmsg()) from either socket (report where the datagram came from to stdout), spawn a detached thread or fork a child to handle the connection with the client. The main thread/parent should return to multiplexing on the listen sockets.
Note: You may use detached threads or child processes.  Threads can have concurrency issues depending on your implementation.
The thread/child will create a new UDP socket with an ephemeral port. Use getsockname(2) to print out the port and address of this new socket. 
The thread/child will respond to the client with a connection datagram containing the new ephemeral port that the client and server will communicate on.
The thread/child will need a reference to the IP address that the datagram came in on to send the datagram to the client through the socket, otherwise the client would reject the packet.
This datagram must be acknowledged by the client. It is to retransmitted in the event of loss.
In the event of ACK lost, think about how the client will react also? 
Upon receiving an acknowledgement on this new socket from the client, the thread/child closes any socket inherited from the main thread/parent which is no longer required and sends the client the file or file list through sequenced UDP datagrams on the ‘new’ socket.

UDP Reliability & Congestion Control
In this assignment you are to implement a subset of the reliability mechanisms from on TCP Reno. 
Reliable data transmissions using ARQ sliding windows
Client uses Fast Retransmit and Cumulative ACKs
Congestion Control
SlowStart
Congestion Avoidance (Additive-Increase/Multiplicative Decrease - AIMD)

Some of the details required are outlined below:
Use sequence numbers per packet, not by bytes as done in TCP. 
Each datagram payload is a fixed 512 bytes, inclusive of your own header which should include at least the following
Sequence number
Cumulative ACK
Receiver window advertisements
Implement a timeout mechanism on the sender (server). Use Stevens, Section 22.5 book code (rtt/dg_cli.c) as a starting place. You will need to modify for the code send-send-send-...  rather than send-receive, send-receive, …
You MUST use the unprtt.h and rtt.c provided on PIAZZA. This code has been modified to use reduced times.
Both sender (server) and receiver (client)  must have sliding windows.
Sender sliding window for retransmission of lost datagrams, with maximum size as specified in the server.in file
Receiver sliding window,  maximum size (in datagrams) as specified in the client.in file,  will be a simple buffer to store incoming sequenced datagrams for the client before printing them to stdout. The receiver must “advertise” to the sender within the ACK packets the current amount of space in the buffer. In this way the server can’t overrun the receiver with packets. 
When either the sender’s or receiver’s sliding window is Full, print out a message on stdout. 
Note: if the receiver buffer fills, there is a potential for deadlock. We will not test/handle this case.
On the receiver’s side (client) will  use a thread to read from the receive buffer and print the contents to stdout. This thread will repetitively loop till all the file contents are printed. In the loop, the thread will sleep for an exponential distribution of -1 * μ * ln (random()). Upon waking, the thread will read and print all in-order fill contents available in the receive buffer at that moment, then sleeps for an exponential distribution amount of time.  
You will need to use locks/semaphores to provide mutual exclusion on the buffer and there is no starvation. 
You must create a mechanism to determine the when the sender had sent the last datagram. You can not use the EOF marker as a designator in the datagram, as it could be misinterpreted. Also not that the last datagram can be a “short” transmission (less than 512 bytes).  When the last datagram is ACK’d the child/server thread terminates.  Make sure to clean up any zombies on the server and client side. 

README file
You must include a README file with your submission which outlines what is properly working and any parts which have issues. Also outline any design choices which are relevant to your grading.

Assignment Output and Grading
Basic output required from your program has been described in the sections above. 
You must provide further output – clear, well-structured, well-laid-out, concise but sufficient and helpful – in the client and server windows. 
The grading TA must be able to trace the correct evolution of your TCP’s behaviour in all its intricacies: 
information (e.g., sequence number) on datagrams and acks sent and dropped
window advertisements
datagram retransmissions (and why :  dup acks or RTO)
entering/exiting Slow Start and Congestion Avoidance
ssthresh and cwnd values
sender and receiver windows locking/unlocking; etc., etc. . . .  .

It is your responsibility to convince us that your Congestion Control and Reliability is working correctly. It is not the TA’s nor my responsibility to sit staring at an essentially blank screen trying to figure out if your implementation is really working correctly in all its very intricate aspects, simply because the transferred file seems to be printing o.k. in the client window. Nor is it our responsibility dig through a mountain of obscure, ill-structured, hyper-messy, debugging-style output because, for example, your effort-conserving concept of what is ‘suitable’ is to dump your debugging output on us, relevant, irrelevant, and everything in between. If we can not decipher your output to have a basic understanding of what is happening in less than 5 mins, you will get a 0.
