#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "GoBackN.h"

/* Connection state. */
int sequence_num;
int ack_number;
float estimated_RTT;
float Dev_RTT;
int closing;
float TimeoutInterval;
int sender_or_receiver;
int sending_mode;

/* Window state. */
int mode; //slow start (0) or congestion avoidance modes (1)
/* Sender specific. */
int send_base; //left most point of our send window
int send_window_size; //size of window
int sender_cur_pos; //current position in send_buf
int send_window_remaining;
struct packet send_buf[WINDOW_SIZE_CAP]; // buffer array to hold the packets we pull in from the sender
/* Receiver specific. */
int recv_base; //left most point of our receiver window
int recv_window_size; //size of receiver window
struct receiver_packet recv_buf[WINDOW_SIZE_CAP]; // buffer array to hold the packets for the receiver


/* The following three helper functions make dealing with time values a bit easier. */
int timeval_to_msec(struct timeval *t) {
    return t->tv_sec*1000+t->tv_usec/1000;
}

void msec_to_timeval(int millis, struct timeval *out_timeval) {
    out_timeval->tv_sec = millis/1000;
    out_timeval->tv_usec = (millis%1000)*1000;
}

int current_msec() {
    struct timeval t;
    gettimeofday(&t,0);
    return timeval_to_msec(&t);
}
/* End time functions. */

int my_socket(int domain, int type, int protocol) {
    /* Initialize connection state. */
    sequence_num = 0;
    ack_number = 0;
    estimated_RTT = 0;
    Dev_RTT = 0;
    estimate_RTT(1000);
    sending_mode = 0;
    
    /* Initialize window state. */
    mode = 0; //start in slow start mode
    /* Sender specific. */
    send_window_size = 4; //start with cwnd of size 4
    send_base = 0; //starting the window at index 0 of our send_buf
    sender_cur_pos = 0; //start at index 0
    send_window_remaining = send_window_size;

    /* Receiver specific. */
    recv_base = 0;
    recv_window_size = 12;

    return socket(domain, type, protocol);
}

int my_connect(int socket,struct sockaddr_in *toaddr,int addrsize) {
    return connect(socket,(struct sockaddr*)toaddr,addrsize);
}

int my_rtt(int socket) {
    return (int)TimeoutInterval;
}

void estimate_RTT(int SampleRTT){
    float a = .125;
    float B = .25;
    Dev_RTT = (1 - B) * Dev_RTT + B * abs(SampleRTT - estimated_RTT);
    estimated_RTT  = (1 - a) * estimated_RTT + a * SampleRTT;
    TimeoutInterval = estimated_RTT + 4*Dev_RTT + 15;
}

void receive_ACK(int socket){
    fprintf(stderr, "receive ack\n");
    int i;
    struct timeval timer;
    int rtt = lowest_timeout()->projected_timeout - current_msec();
    if (rtt <= 0) rtt = 15;
    msec_to_timeval(rtt, &timer);

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(socket, &rfds);

    fprintf(stderr, "rtt = %d\n",rtt);
    int select_ret = select(socket+1,&rfds,NULL,NULL,&timer);
    if (!select_ret) { //timed out need to retransmit packets
        for (i = send_base; i < sender_cur_pos /*send_base + send_window_size*/; i++) {
            int idx = i % WINDOW_SIZE_CAP;
            if(send_buf[idx].ACK == 0){
                send(socket, send_buf[idx].array, send_buf[idx].array_len, 0);
                send_buf[idx].timeout *= 2;
                send_buf[idx].projected_timeout = current_msec() + send_buf[idx].timeout;
                send_buf[idx].packet_status = 1;
            }
        }
        send_window_size /= 2; if (send_window_size < 1) send_window_size = 1;
        send_window_remaining = send_window_size;
        mode = 1;
        receive_ACK(socket);
    }
    else { //has not timed out
        fprintf(stderr, "hasn't timed out\n");
        char recv_buffer[MAX_PACKET];
        struct sockaddr_in fromaddr;
        socklen_t addrlen = sizeof(fromaddr);
        memset(recv_buffer,0,sizeof(recv_buffer));
        recvfrom(socket,recv_buffer,MAX_PACKET,0,(struct sockaddr *) &fromaddr, &addrlen); 

        fprintf(stderr, "successful receive\n");
        struct packet_hdr *hdr = (struct packet_hdr *) recv_buffer;
        int ack_num = ntohl(hdr->ack_number);
        send_buf[ack_num].ACK = 1;

        // update RTT if not retransmitted
        int has_been_retransmitted = 0;
        for (i = send_base; i < ack_num; i++) {
            int idx = i % WINDOW_SIZE_CAP;
            if (send_buf[idx].packet_status == 1) 
                has_been_retransmitted = 1;
        }
        if (!has_been_retransmitted) {
            int sample = rtt-timeval_to_msec(&timer);
            estimate_RTT(sample);
        }

        if (ack_num < send_base) { //ignore packet
            receive_ACK(socket);
        }
        else {
            send_buf[ack_num].ACK = 1;
            //int advance = ack_num - send_base;
            AIMD();
        }
    }
}
struct packet * lowest_timeout() {
    int i;
    int min_time = 999999999;
    int min_index = send_base % WINDOW_SIZE_CAP;
    for (i = send_base; i < sender_cur_pos; i++) {
        int idx = i % WINDOW_SIZE_CAP;
        if (send_buf[idx].projected_timeout< min_time) {
            min_time = send_buf[idx].projected_timeout;
            min_index = idx;
        }
    }
    return &send_buf[min_index];
}
void AIMD(){
    //send_window_remaining -= advance;
    //send_base += advance;
    while(send_buf[send_base].ACK == 1){
        send_base++;
        send_window_remaining--;
    }
    
    if (send_window_remaining <= 0) {
        if (mode == 0) { //slow start
            send_window_size *= 2;
            send_window_remaining = send_window_size;
        }
        else { //congestion avoidance
            send_window_size++;
            send_window_remaining = send_window_size;
        }
    }
}

void my_send(int sock, void *buf, size_t len)
{
    sender_or_receiver = 1;
    if(len == 0){
        closing = 1;
    }

    if (sender_cur_pos >= send_base + send_window_size) { //window is full
        receive_ACK(sock);
    }
    memset(send_buf[sender_cur_pos].array,0,sizeof(send_buf[sender_cur_pos].array));
    struct packet_hdr *hdr = (struct packet_hdr *) send_buf[sender_cur_pos].array;
    memcpy(hdr+1,buf,len);
    send_buf[sender_cur_pos].array_len = sizeof(struct packet_hdr) + len;
    send_buf[sender_cur_pos].timeout = my_rtt(sock);
    send_buf[sender_cur_pos].sequence_number = sequence_num;
    send_buf[sender_cur_pos].packet_status = 0;
    send_buf[sender_cur_pos].projected_timeout = current_msec() + send_buf[sender_cur_pos].timeout;
    send_buf[sender_cur_pos].ACK = 0;
    send_buf[sender_cur_pos].FIN = 0;
    
    hdr->sequence_number = htonl(sequence_num);
    hdr->close = htonl(0);
    
    send(sock, send_buf[sender_cur_pos].array, sizeof(struct packet_hdr) + len, 0);
    fprintf(stderr, "Sending seq num %d: ", ntohl(hdr->sequence_number));
    sequence_num++;
    sender_cur_pos++;
}

int my_recv(int sock, void *buf, size_t length) {
    sender_or_receiver = 0;
    /* Buffer for the packet. */
    char packet[MAX_PACKET];
    memset(packet, 0, sizeof(packet));

    while (1) {

        if(recv_buf[recv_base].exists == 1){
            int bytesize = recv_buf[recv_base].array_len;
            memcpy(buf, recv_buf[recv_base].array, bytesize);
            recv_base++;
            return bytesize;
        }

        struct packet_hdr *hdr = (struct packet_hdr *) packet;

        struct sockaddr_in fromaddr;
        socklen_t addrlen = sizeof(fromaddr);
        int recv_count = recvfrom(sock, packet, MAX_PACKET, 0,
                                (struct sockaddr *) &fromaddr, &addrlen);

        /* Associate this UDP socket with a remote address so that we can send
         * an ACK packet back to the sender. */
        if(connect(sock, (struct sockaddr *) &fromaddr, addrlen)) {
            perror("connect (my_recv)");
        }

        int seq_num = ntohl(hdr->sequence_number);
        //int close_flag = ntohl(hdr->close);

        fprintf(stderr, "Got packet %d, my ack = %d\n", seq_num, seq_num);

        char ack_packet[MAX_PACKET];
        memset(ack_packet, 0, sizeof(ack_packet));
        struct packet_hdr *ack_hdr = (struct packet_hdr *) ack_packet;
        //ack_hdr->close = htonl(close_flag);

        if (seq_num > recv_base || seq_num == recv_base){ // must buffer this packet

            /* Put this packet into our buffer*/
            memset(recv_buf[seq_num].array,0,sizeof(recv_buf[seq_num].array));
            //struct packet_hdr *hdr = (struct packet_hdr *) recv_buf[seq_num].array;
            memcpy(recv_buf[seq_num].array,packet + sizeof(struct packet_hdr),recv_count - sizeof(struct packet_hdr));

            recv_buf[seq_num].array_len = recv_count - sizeof(struct packet_hdr);
            recv_buf[seq_num].exists = 1;
        }
        ack_hdr->ack_number = htonl(seq_num);
        send(sock,ack_hdr,sizeof(struct packet_hdr),0);
    }
}

int my_close(int sock) {

    //Sender
    if(sender_or_receiver == 1){
        while(send_base < sender_cur_pos){ // take in all unacknowledged packets still in our window
            receive_ACK(sock);    
        }
        my_send(sock, NULL, 0); // sends a FIN message to let the receiver transition into close state
        receive_ACK(sock); // receive the ACK from the receiver for this FIN message
    }

    //Receiver
    else{
        while(1){
            struct timeval timer;
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
            msec_to_timeval(2000, &timer); // extended timeout to ensure sender doesn't get stuck

            /* Manually create final packet to send. */
            char ack_packet[MAX_PACKET];
            memset(ack_packet, 0, sizeof(ack_packet));
            struct packet_hdr *ack_hdr = (struct packet_hdr *) ack_packet;
            ack_hdr->ack_number = htonl(++ack_number);

            send(sock, ack_packet, sizeof(struct packet_hdr), 0);

            int select_ret = select(sock+1, &rfds, NULL, NULL, &timer);

            if(select_ret){ // if we get the final ACK from the sender, close; else resend via loop
                break;
            }
        }
    }
    return close(sock);
}