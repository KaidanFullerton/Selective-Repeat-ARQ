#include <sys/time.h>

#define MAX_PACKET 1400
#define MAX_SEGMENT (MAX_PACKET-sizeof(struct packet_hdr))
#define WINDOW_SIZE_CAP 2000

/* stores header specific packet data */
struct packet_hdr {
    uint32_t sequence_number;
    uint32_t ack_number;
    uint32_t FIN;           /* indicates if packet is a FIN message */
};

/* stores sender specific packet data */
struct packet{
 char array[MAX_PACKET];    /* store packet header + packet data */
 int array_len;             /* byte length of packet header + packet data */
 int timeout;               /* timeout time for this given packet */
 uint32_t sequence_number;
 int packet_status;         /* indicates if packet has been retransmitted */
 int projected_timeout;     /* time in milliseconds, that the packet is expected to timeout */
 int ACK;                   /* indicates if packet has been acknowledged */
 int FIN;                   /* indicates if packet is a FIN message */
};

/* stores recevier specific packet data */
struct receiver_packet{
 char array[MAX_PACKET];    /* store just the packet data */
 int array_len;             /* byte length of packet data */
 uint32_t acknowledgement_number;
 int exists;                /* indicates if the packet slot in the receiver buffer is filled or not */
};

/*
 *   Initalizes all the relevant state needed between the sender and receiver.
 *
 *   @param domain:   specifies a communication domain; selects the protocol family which will be used for communication
 *   @param type:     specifies the communication semantics
 *   @param protocol: specifies a particular protocol to be used with the socket
 *
 *   @return a file descriptor for the new socket 
 */
int my_socket(int domain, int type, int protocol);

/*
 *   Connects the socket referred to by the file descriptor, socket, to the address specificed by toaddr.
 *
 *   @param socket:   file descriptor
 *   @param toaddr:   address to which datagrams are sent, and the only address from which they are received
 *   @param addrsize: size of toaddr
 *
 *   @return If binding succeeds, zero is returned. On error, -1 is returned
 */
int my_connect(int socket,struct sockaddr_in *toaddr,int addrsize);

/*
 *   Transmitts a message to another socket.
 *
 *   @param sock:   file descriptor of sending socket
 *   @param buf:    array in which the message is stored
 *   @param len:    byte length of message
 *
 *   @return a file descriptor for the new socket 
 */
void my_send(int sock, void *buf, size_t len);

/*
 *   Receive a message from a socket
 *
 *   @param sock:   file descriptor to receive from
 *   @param buf:    buffer to store message in
 *   @param length: maximum length to receive
 *
 *   @return byte length of message received into buf
 */
int my_recv(int sock, void *buf, size_t length);

/*
 *   Closes the socket connection using three-way FIN handshake
 *
 *   @param sock:   file descriptor to close
 *
 *   @return returns zero on success. On error, -1 is returned
 */
int my_close(int sock);

/*
 *   Returns the current round trip time estimate for packets being sent
 *
 *   @param sock:   file descriptor for the socket calling this function
 *
 *   @return the time, in milliseconds, that an ACK is expected for a given packet
 */
int my_rtt(int sock);

/*
 *   Helper function to convert a timeval struct to into milliseconds
 *
 *   @param t:   timeval struct used in call to select()
 *
 *   @return time value in milliseconds
 */
int timeval_to_msec(struct timeval *t);

/*
 *   Helper function to set a millisecond timer into a timeval struct
 *
 *   @param millis:        time value, in milliseconds, the timeval struct should store
 *   @param out_timeval:   timeval struct you wish to store a timer value in
 */
void msec_to_timeval(int millis, struct timeval *out_timeval);

/*
 *   Helper function to grab how much runtime, in milliseconds, has elapsed
 *
 *   @return the runtime, in milliseconds, that has elapsed
 */
int current_msec();

/*
 *   Modifies the estimated round trip time, based on a RTT sample, using an exponentially weighted moving average
 *
 *   @param sampleRTT:   sample RTT from a non-retransmitted packet
 */
void estimate_RTT(int SampleRTT);

/*
 *   Helper function to handle the receiving of acknowledgements for sent packets
 */
void receive_ACK();

/*
 *   Determines the packet with the smallest timeout value in our sending window
 *
 *   @return pointer to the packet with the smallest timeout value
 */
struct packet * lowest_timeout();

/*
 *   Manages our Congestion Window according to Additive Increase Multiplicative Decrease specifications. 
 *   This is ran after each acknowledged packet.
 */
void AIMD();