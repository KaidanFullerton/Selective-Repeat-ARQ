#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SelectiveRepeat.h"

int main(int argc, char** argv) {
    if(argc < 3) {
        fprintf(stderr,"Usage: sender <remote host> <port>\n");
        return 1;
    }

    /* Create a UDP socket. */
    int sock = my_socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        perror("my_socket");
        return 1;
    }

    /* Bind our UDP socket to the user-supplied port. */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock,(struct sockaddr*)&addr,sizeof(addr))) {
        perror("bind");
        return 1;
    }

    /* Associate this UDP socket with a remote address. */
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    if(my_connect(sock, &addr,sizeof(addr))) {
        perror("my_connect");
        return 1;
    }

    char buf[MAX_SEGMENT];
    memset(buf,0,sizeof(buf));

    int totalbytes = 0;
    int readbytes;

    /* Record the start time. */
    int starttime = current_msec();

    /* Keep reading MAX_SEGMENT size chunks from stdin and sending them until
     * there's nothing left. */
    while((readbytes = fread(buf, 1, sizeof(buf), stdin)) != 0) {
        totalbytes += readbytes;
        my_send(sock, buf, readbytes);
        printf("sent a packet\n");
    }

    /* Record the end time. */
    int finished_msec = current_msec();

    fprintf(stderr,"\nFinished sending, closing socket.\n");
    if (my_close(sock)) {
        perror("close");
        return 1;
    }

    fprintf(stderr,"\nSent %d bytes in %.4f seconds, %.2f kB/s\n",
                    totalbytes,
                    (finished_msec-starttime)/1000.0,
                    totalbytes/(float)(finished_msec-starttime));
    fprintf(stderr,"Estimated RTT: %d ms\n",
                    my_rtt(sock));

    /* Done.  Return success. */
    return 0;
}
