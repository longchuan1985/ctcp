#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>

typedef int bool;
#define TRUE 1
#define FALSE 0
#define MIN(x,y) (y)^(((x) ^ (y)) &  - ((x) < (y)))
#define MAX(x,y) (y)^(((x) ^ (y)) & - ((x) > (y)))

// flags for Data_Pckt
typedef enum {NORMAL=0, SYN, SYN_ACK, FIN, FIN_ACK, CODED} flag_t;

//---------------- CTCP parameters ------------------//
#define MSS 1500
#define IPUDP_HEADER_SIZE 32 // We have to take into account the IP+UDP header
                             // Max IP header is 24B, UDP header is 8B 

#define CHECKSUM_SIZE 16 // MD5 is a 16 byte checksum
// TODO: change such that we can change MSS, BLOCK_SIZE, CODING_WIN via config file

#define GF 256  // field size for coding, options are 2 or 256
#define BLOCK_SIZE 32 // Maximum # of packets in a block (Default block length)
#define NUM_BLOCKS 100
#define MAX_CWND 1000 
#define MAX_CODING_WND 32 

#define ACK_SIZE sizeof(double) \
    + sizeof(int)               \
    + sizeof(uint32_t)          \
    + sizeof(uint32_t)          \
    + sizeof(uint8_t)           

#define PAYLOAD_SIZE (MSS \
  - IPUDP_HEADER_SIZE \
  - (sizeof(double) \
   + sizeof(flag_t) \
   + sizeof(uint32_t) \
   + sizeof(uint32_t) \
   + sizeof(uint8_t) \
   + sizeof(uint8_t) \
   + sizeof(uint8_t) \
   ) ) 

typedef int socket_t;
typedef struct timeval timeval_t;

typedef struct{
  double tstamp;
  flag_t flag;
  uint32_t  seqno; // Sequence # of the coded packet sent
  uint32_t  blockno; // Base of the current block
  uint8_t start_packet; // The # of the first packet that is mixed
  uint8_t  num_packets; // The number of packets that are mixed
  uint8_t packet_coeff; // Seed for coefficients of the mixed packets
  //  unsigned char checksum[CHECKSUM_SIZE];  // MD5 checksum
  char payload[PAYLOAD_SIZE];  
} Data_Pckt;

typedef struct{
  double tstamp;
  flag_t flag;
  uint32_t  ackno; // The sequence # that is being acked --> this is to make it Reno-like
  uint32_t  blockno; // Base of the current block
  uint8_t dof_rec;  // Number of dofs left from the block
  //  unsigned char checksum[CHECKSUM_SIZE];  // MD5 checksum
} Ack_Pckt;

// used for zero copy 
typedef union {
   Data_Pckt msg;
   Ack_Pckt ack;
   char buff[MSS];
} Msgbuf;

// used to provide pool of packets for mem management
typedef struct Skb {
   Msgbuf msgbuf;
   struct Skb* next;
   bool used;
} Skb;

typedef struct{
  uint16_t payload_size;
  char* payload;
} Bare_Pckt; // This is the datastructure for holding packets before encoding

// -------------- MultiThreading related variables ------------//

typedef struct{ 
  pthread_mutex_t block_mutex;
  pthread_cond_t  block_free_condv;
  //pthread_cond_t  block_ready_condv;
  uint32_t len; // Number of bare packets inside the block
  char* content[BLOCK_SIZE]; // Array of pointers that point to the marshalled data of the bare packets
  Skb* skb[BLOCK_SIZE]; // for memory management
} Block_t;

typedef struct{
  uint8_t dofs; // Number of degrees of freedom thus far
  uint8_t len;
  char rows[BLOCK_SIZE][BLOCK_SIZE]; // Matrix of the coefficients of the coded packets
  char* content[BLOCK_SIZE]; // Contents of the coded packets
  int row_len[BLOCK_SIZE];  // maximum number of non-zeros in each row
  int max_coding_wnd; //largest number of packets mixed together
  int dofs_pushed;
  int max_packet_index;
  Skb* skb[BLOCK_SIZE]; // for memory management
} Coded_Block_t;

double getTime(void);
Data_Pckt* dataPacket(uint32_t seqno, uint32_t blockno, uint8_t num_packets);
Skb* ackPacket(uint32_t ackno, uint32_t blockno, uint8_t dofs_left, int debug);
void htonpData(Data_Pckt *msg);
void htonpAck(Ack_Pckt *msg);
void ntohpData(Data_Pckt *msg);
void ntohpAck(Ack_Pckt *msg);
void prettyPrint(char** coeffs, int window);
uint8_t FFmult(uint8_t x, uint8_t y);
uint8_t xFFlog(uint8_t x);
uint8_t fastFFmult(uint8_t x, uint8_t logy);
void seedfastrand(uint32_t seed);
uint32_t fastrand();
Skb* alloc_skb(int debug);
void free_skb(void* skb);

#ifdef __MACH__
#include <sys/time.h>
#endif
//portable clock_gettime 
#define CLOCK_REALTIME  0 // dummy constant
int clock_gettime(int dummy, struct timespec* t);

#ifdef __MACH__
int pthread_timedjoin_np(pthread_t td, void **res, struct timespec *ts);
#endif

#endif // UTIL_H_
