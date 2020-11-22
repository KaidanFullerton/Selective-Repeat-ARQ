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
#include "GoBackN.h"

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr,"Usage: receiver <listening port>\n");
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
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;
    int res = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    if(res < 0) {
        perror("bind");
        return 1;
    }

    int totalbytes = 0;
    int starttime = 0;

    char segment[MAX_SEGMENT];
    while(1) {
        int recv_count = my_recv(sock, segment, MAX_SEGMENT);

        if (recv_count == 0)
            /* We've reached the end, stop receiving. */
            break;

        if (totalbytes == 0)
            /* If this is the first iteration, record the start time. */
            starttime = current_msec();

        totalbytes += recv_count;

        /* Write the payload to stdout. */
        fwrite(segment, 1, recv_count, stdout);
        fflush(stdout);
    }

    /* Record the time we finished. */
    int finished_msec = current_msec();

    fprintf(stderr,"\nReceived %d bytes in %.4f seconds, %.2f kB/s\n",
                    totalbytes,
                    (finished_msec-starttime)/1000.0,
                    totalbytes/(float)(finished_msec-starttime));

    fprintf(stderr, "\nFinished receiving file, closing socket.\n");
    if (my_close(sock)) {
        perror("close");
        return 1;
    }

    fflush(stdout);

    return 0;
}
