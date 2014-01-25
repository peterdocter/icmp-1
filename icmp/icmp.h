#ifndef __ICMP_H__
#define __ICMP_H__

struct icmphdr {
   uint8_t   type;          // See 'ICMP Message Types' below and RFC 792
   uint8_t   code;          // See 'Codes' below and RFC 792
   uint16_t  checksum;      // IP style one's complement checksum
   uint16_t  id;            // Unique identifier for sender - e.g. process id
   uint16_t  seqNum;        // ...to identify individual request
   uint32_t  data[ ];       // Response includes ICMP message (8 bytes) sent by client
} __attribute__((__packed__));


char * messages[ ] = {
   "Echo reply",
   "Type 1",
   "Type 2",
   "Destination unreachable",
   "Source quench",
   "Redirect",
   "Type 6",
   "Type 7",
   "Echo request",
   "Router advertisement",
   "Router discovery",
   "Time exceeded",
   "Parameter problem",
   "Timestamp request",
   "Timestamp reply",
   "Information request (obsol.)",
   "Information reply (obsol.)",
   "Address mask request",
   "Address mask reply"
};

// ICMP Message Types
#define ICMP_ECHOREPLY          0       // Echo Reply
#define ICMP_DEST_UNREACH       3       // Destination Unreachable
#define ICMP_SOURCE_QUENCH      4       // Source Quench
#define ICMP_REDIRECT           5       // Redirect (change route)
#define ICMP_ECHO               8       // Echo Request
#define ICMP_TIME_EXCEEDED      11      // Time Exceeded
#define ICMP_PARAMETERPROB      12      // Parameter Problem
#define ICMP_TIMESTAMP          13      // Timestamp Request
#define ICMP_TIMESTAMPREPLY     14      // Timestamp Reply
#define ICMP_INFO_REQUEST       15      // Information Request
#define ICMP_INFO_REPLY         16      // Information Reply
#define ICMP_ADDRESS            17      // Address Mask Request
#define ICMP_ADDRESSREPLY       18      // Address Mask Reply
#define NR_ICMP_TYPES           18


// Codes for ICMP_DEST_UNREACH
#define ICMP_NET_UNREACH        0       // Network Unreachable
#define ICMP_HOST_UNREACH       1       // Host Unreachable
#define ICMP_PROT_UNREACH       2       // Protocol Unreachable
#define ICMP_PORT_UNREACH       3       // Port Unreachable
#define ICMP_FRAG_NEEDED        4       // Fragmentation Needed/DF set
#define ICMP_SR_FAILED          5       // Source Route failed
#define ICMP_NET_UNKNOWN        6
#define ICMP_HOST_UNKNOWN       7
#define ICMP_HOST_ISOLATED      8
#define ICMP_NET_ANO            9
#define ICMP_HOST_ANO           10
#define ICMP_NET_UNR_TOS        11
#define ICMP_HOST_UNR_TOS       12
#define ICMP_PKT_FILTERED       13      // Packet filtered
#define ICMP_PREC_VIOLATION     14      // Precedence violation
#define ICMP_PREC_CUTOFF        15      // Precedence cut off
#define NR_ICMP_UNREACH         15      // instead of hardcoding immediate value

// Codes for ICMP_REDIRECT
#define ICMP_REDIR_NET          0       // Redirect Net
#define ICMP_REDIR_HOST         1       // Redirect Host
#define ICMP_REDIR_NETTOS       2       // Redirect Net for TOS
#define ICMP_REDIR_HOSTTOS      3       // Redirect Host for TOS

// Codes for ICMP_TIME_EXCEEDED
#define ICMP_EXC_TTL            0       // TTL count exceeded
#define ICMP_EXC_FRAGTIME       1       // Fragment Reass time exceeded

#endif // __ICMP_H__
