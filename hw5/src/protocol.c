#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "csapp.h"
#include "protocol.h"
#include "debug.h"
/*
 * Send a packet, which consists of a fixed-size header followed by an
 * optional associated data payload.
 *
 * @param fd  The file descriptor on which packet is to be sent.
 * @param hdr  The fixed-size packet header, with multi-byte fields
 *   in network byte order
 * @param data  The data payload, or NULL, if there is none.
 * @return  0 in case of successful transmission, -1 otherwise.
 *   In the latter case, errno is set to indicate the error.
 *
 * All multi-byte fields in the packet are assumed to be in network byte order.
 * 
 * Remember that it is always possible for read() and write()
    to read or write fewer bytes than requested.  You must check for and
    handle these "short count" situations.
 */
int proto_send_packet(int fd, JEUX_PACKET_HEADER *hdr, void *data) {

    if (rio_writen(fd, hdr, sizeof(JEUX_PACKET_HEADER)) < 0) {
        return -1;
    }
    debug("sent packet header");
    debug("hdr->id: %d", hdr->id);
    debug("hdr->type: %d", hdr->type);
    debug("hdr->role: %d", hdr->role);
    debug("hdr->size: %d", hdr->size);
    debug("hdr->timestamp_sec: %d", hdr->timestamp_sec);
    debug("hdr->timestamp_nsec: %d", hdr->timestamp_nsec);
    // Convert the header fields to network byte order
    // hdr->size = htons(hdr->size);
    // hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    // hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

    if (htons(hdr->size) > 0 && data) {
        if (rio_writen(fd, data, htons(hdr->size)) < 0) {
            return -1;
        }
    }

    return 0;
}

int proto_recv_packet(int fd, JEUX_PACKET_HEADER *hdr, void **payloadp) {
    
    if (rio_readn(fd, hdr, sizeof(JEUX_PACKET_HEADER)) <= 0) {
        return -1;
    }
    // Convert the header fields to host byte order
    // hdr->size = ntohs(hdr->size);
    // hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    // hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

    // create a buffer for the payload
    void *payload = NULL;
    // Read the payload
    if (ntohs(hdr->size) > 0) {
        payload = malloc(ntohs(hdr->size) + 1);
        if (!payload) {
            return -1;
        }
        // set the last byte to null
        ((char *)payload)[ntohs(hdr->size)] = '\0';
        if (rio_readn(fd, payload, ntohs(hdr->size)) < 0) {
            free(payload);
            return -1;
        }
    }

    *payloadp = payload;
    return 0;
}
