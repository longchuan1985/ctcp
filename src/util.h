#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>

typedef int bool;
#define TRUE 1
#define FALSE 0
#define MIN(x,y) (y)^(((x) ^ (y)) &  - ((x) < (y)))
#define MAX(x,y) (y)^(((x) ^ (y)) & - ((x) > (y)))

// flags for Data_Pckt
typedef enum {NORMAL=0, EXT_MOD, FIN_CLI, PARTIAL_BLK, OLD_PKT} flag_t;

//---------------- CTCP parameters ------------------//
#define MSS 1500 // XXX: make sure that this is fine...
#define CHECKSUM_SIZE 16 // MD5 is a 16 byte checksum
#define PAYLOAD_SIZE 1400
// TODO: change such that we can change PAYLOAD_SIZE, BLOCK_SIZE, CODING_WIN via config file
#define BLOCK_SIZE 128 // Maximum # of packets in a block (Default block length)
#define CODING_WND 3
#define MAX_CWND 70

#define ACK_SIZE sizeof(double) \
    + sizeof(int)               \
    + sizeof(uint32_t)          \
    + sizeof(uint32_t)          \
    + sizeof(uint8_t)

typedef int socket_t;
typedef struct timeval timeval_t;

typedef struct{
  double tstamp;
  flag_t flag;
  uint32_t  seqno; // Sequence # of the coded packet sent
  uint32_t  blockno; // Base of the current block
  uint8_t blk_len; // The number of packets in the block
  uint8_t start_packet; // The # of the first packet that is mixed
  uint8_t  num_packets; // The number of packets that are mixed
  uint8_t* packet_coeff; // The coefficients of the mixed packets
  //  unsigned char checksum[CHECKSUM_SIZE];  // MD5 checksum
  char *payload;  // The payload size is negotiated at the beginning of the transaction
} Data_Pckt;

typedef struct{
  uint16_t payload_size;
  char* payload;
} Bare_Pckt; // This is the datastructure for holding packets before encoding

// -------------- MultiThreading related variables ------------//

typedef struct{ // TODO: this datastructure can store the dof's and other state related to the blocks
  pthread_mutex_t block_mutex;
  uint32_t len; // Number of bare packets inside the block
  char** content; // Array of pointers that point to the marshalled data of the bare packets
} Block_t;

typedef struct{
  uint32_t blockno;
  int dof_request;
} coding_job_t;

typedef struct{
  double tstamp;
  flag_t flag;
  uint32_t  ackno; // The sequence # that is being acked --> this is to make it Reno-like
  uint32_t  blockno; // Base of the current block
  uint8_t dof_req;  // Number of dofs left from the block
  //  unsigned char checksum[CHECKSUM_SIZE];  // MD5 checksum
} Ack_Pckt;

typedef struct{
  uint8_t dofs; // Number of degrees of freedom thus far
  uint8_t len;
  char** rows; // Matrix of the coefficients of the coded packets
  char** content; // Contents of the coded packets
} Coded_Block_t;

double getTime(void);
Data_Pckt* dataPacket(uint32_t seqno, uint32_t blockno, uint8_t num_packets);
Ack_Pckt* ackPacket(uint32_t ackno, uint32_t blockno, uint8_t dofs_left);
void htonpData(Data_Pckt *msg);
void htonpAck(Ack_Pckt *msg);
void ntohpData(Data_Pckt *msg);
void ntohpAck(Ack_Pckt *msg);
void prettyPrint(char** coeffs, int window);
uint8_t FFmult(uint8_t x, uint8_t y);

#endif // UTIL_H_
