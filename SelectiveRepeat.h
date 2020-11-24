#include <sys/time.h>


#define MAX_PACKET 1400
#define MAX_SEGMENT (MAX_PACKET-sizeof(struct packet_hdr))

#define WINDOW_SIZE_CAP 2000

struct packet_hdr {
    uint32_t sequence_number;
    uint32_t ack_number;
    uint32_t close;
};

struct packet{
 char array[MAX_PACKET]; //hold the header +  packet.
 int array_len;
 int timeout; //future timeout per packet
 uint32_t sequence_number;
 int packet_status; //retransmitted or not

 int projected_timeout; //time the packet was sent, computed by current_msec() 
 int ACK; //if an ACK or not
 int FIN; //if a FIN or not: can help us to skip it in window if other packets are available to send
};

struct receiver_packet{
 char array[MAX_PACKET]; //hold the header +  packet.
 int array_len;
 uint32_t acknowledgement_number;
 int exists;
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