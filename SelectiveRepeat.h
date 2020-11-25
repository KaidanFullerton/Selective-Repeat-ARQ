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

int my_socket(int domain, int type, int protocol);
int my_connect(int socket,struct sockaddr_in *toaddr,int addrsize);
void my_send(int sock, void *buf, size_t len);
int my_recv(int sock, void *buf, size_t length);
int my_close(int sock);
int my_rtt(int sock);

int timeval_to_msec(struct timeval *t);
void msec_to_timeval(int millis, struct timeval *out_timeval);
int current_msec();

void estimate_RTT(int SampleRTT);

//GoBackN specific helper functions
void receive_ACK(); //helper function to handle receving of ACKs for sent packets
struct packet * lowest_timeout(); //determines the packet with the lowest timeout value in our sending window
void AIMD(); //manages our cwnd according to Additive Increase Multiplicative Decrease specifications