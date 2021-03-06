#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "clictcp.h"


uint8_t inv_vec[256]={
  0x00, 0x01 ,0x8d ,0xf6 ,0xcb ,0x52 ,0x7b ,0xd1 ,0xe8 ,0x4f ,0x29 ,0xc0 ,0xb0 ,0xe1 ,0xe5 ,0xc7,
  0x74 ,0xb4 ,0xaa ,0x4b ,0x99 ,0x2b ,0x60 ,0x5f ,0x58 ,0x3f ,0xfd ,0xcc ,0xff ,0x40 ,0xee ,0xb2 ,
  0x3a ,0x6e ,0x5a ,0xf1 ,0x55 ,0x4d ,0xa8 ,0xc9 ,0xc1 ,0x0a ,0x98 ,0x15 ,0x30 ,0x44 ,0xa2 ,0xc2 ,
  0x2c ,0x45 ,0x92 ,0x6c ,0xf3 ,0x39 ,0x66 ,0x42 ,0xf2 ,0x35 ,0x20 ,0x6f ,0x77 ,0xbb ,0x59 ,0x19 ,
  0x1d ,0xfe ,0x37 ,0x67 ,0x2d ,0x31 ,0xf5 ,0x69 ,0xa7 ,0x64 ,0xab ,0x13 ,0x54 ,0x25 ,0xe9 ,0x09 ,
  0xed ,0x5c ,0x05 ,0xca ,0x4c ,0x24 ,0x87 ,0xbf ,0x18 ,0x3e ,0x22 ,0xf0 ,0x51 ,0xec ,0x61 ,0x17 ,
  0x16 ,0x5e ,0xaf ,0xd3 ,0x49 ,0xa6 ,0x36 ,0x43 ,0xf4 ,0x47 ,0x91 ,0xdf ,0x33 ,0x93 ,0x21 ,0x3b ,
  0x79 ,0xb7 ,0x97 ,0x85 ,0x10 ,0xb5 ,0xba ,0x3c ,0xb6 ,0x70 ,0xd0 ,0x06 ,0xa1 ,0xfa ,0x81 ,0x82 ,
  0x83 ,0x7e ,0x7f ,0x80 ,0x96 ,0x73 ,0xbe ,0x56 ,0x9b ,0x9e ,0x95 ,0xd9 ,0xf7 ,0x02 ,0xb9 ,0xa4 ,
  0xde ,0x6a ,0x32 ,0x6d ,0xd8 ,0x8a ,0x84 ,0x72 ,0x2a ,0x14 ,0x9f ,0x88 ,0xf9 ,0xdc ,0x89 ,0x9a ,
  0xfb ,0x7c ,0x2e ,0xc3 ,0x8f ,0xb8 ,0x65 ,0x48 ,0x26 ,0xc8 ,0x12 ,0x4a ,0xce ,0xe7 ,0xd2 ,0x62 ,
  0x0c ,0xe0 ,0x1f ,0xef ,0x11 ,0x75 ,0x78 ,0x71 ,0xa5 ,0x8e ,0x76 ,0x3d ,0xbd ,0xbc ,0x86 ,0x57 ,
  0x0b ,0x28 ,0x2f ,0xa3 ,0xda ,0xd4 ,0xe4 ,0x0f ,0xa9 ,0x27 ,0x53 ,0x04 ,0x1b ,0xfc ,0xac ,0xe6 ,
  0x7a ,0x07 ,0xae ,0x63 ,0xc5 ,0xdb ,0xe2 ,0xea ,0x94 ,0x8b ,0xc4 ,0xd5 ,0x9d ,0xf8 ,0x90 ,0x6b ,
  0xb1 ,0x0d ,0xd6 ,0xeb ,0xc6 ,0x0e ,0xcf ,0xad ,0x08 ,0x4e ,0xd7 ,0xe3 ,0x5d ,0x50 ,0x1e ,0xb3 ,
  0x5b ,0x23 ,0x38 ,0x34 ,0x68 ,0x46 ,0x03 ,0x8c ,0xdd ,0x9c ,0x7d ,0xa0 ,0xcd ,0x1a ,0x41 ,0x1c
};




/*
 * Handler for when the user sends the signal SIGINT by pressing Ctrl-C
 */
void
ctrlc(clictcp_sock *csk){
  csk->end_time = csk->end_time - csk->start_time;  /* elapsed time */

  if (csk->end_time==0) csk->end_time = 0.1;

}

clictcp_sock*
connect_ctcp(char *host, char *port, char *lease_file, struct child_local_cfg* cfg){
    int optlen,rlth;
    struct addrinfo *result;
    int k; // for loop counter
    int rv;
    int numbytes;//[MAX_SUBSTREAMS];
    int rcvspace = 0;


    // Create the ctcp socket
    clictcp_sock* csk = create_clictcp_sock(cfg);

    dhcp_lease leases[MAX_SUBSTREAMS];
    if (lease_file != NULL){
      // Need to make sure there is at least one lease in the lease file
      csk->substreams = readLease(lease_file, leases);
    }

    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // This works for buth IPv4 and IPv6
    hints.ai_socktype = SOCK_DGRAM;

    if((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return NULL;
    }

    for(result = servinfo; result != NULL; result = result->ai_next) {
      
      if((csk->sockfd[0] = socket(result->ai_family,
                                  result->ai_socktype,
                                  result->ai_protocol)) == -1){
        perror("Failed to initialize socket");
        continue;
      }
      for (k = 1; k < csk->substreams; k++){
        csk->sockfd[k] = socket(result->ai_family,
                                result->ai_socktype,
                                result->ai_protocol);
      }

      break;
    }

    csk->srv_addr = *(result->ai_addr);

    //printf("*** port: %d *** \n",  ((struct sockaddr_in*)&(csk->srv_addr))->sin_port);
    //printf("*** addr: %s *** \n",  inet_ntoa( ((struct sockaddr_in*)&(csk->srv_addr))->sin_addr));

    // If we got here, it means that we couldn't initialize the socket.
    if(result  == NULL){
      perror("Failed to create socket");
      return NULL;
    }
    //freeaddrinfo(servinfo);


    //-------------- BIND to proper local address ----------------------
    // Only bind if the interface IP address is specified through the command line

    if (lease_file != NULL){

      struct addrinfo *result_cli, *cli_info;
      int opt = 1;
    
      for (k=0; k < csk->substreams; k++){
      
        //        make_new_table(&leases[k], k+1, k+1);

        if((rv = getaddrinfo(leases[k].address, NULL, &hints, &cli_info)) != 0) {
          fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
          return NULL;
        }

        for(result_cli = cli_info; result_cli != NULL; result_cli = result_cli->ai_next) {
          
          //          printf("IP address trying to bind to %s \n\n", inet_ntoa(((struct sockaddr_in*)result_cli->ai_addr)->sin_addr));
     
          setsockopt(csk->sockfd[k], IPPROTO_IP, IP_PKTINFO, (void *)&opt, sizeof(opt));

          csk->ifc_addr[k] = (struct sockaddr*) malloc(sizeof(struct sockaddr));
          *(csk->ifc_addr[k]) = *(result_cli->ai_addr);


        }

      }
    }

    //--------------DONE Binding-----------------------------------------
    
    open_status_log(csk, port);


    //signal(SIGINT, (__sighandler_t) ctrlc);

    if (!rcvspace) rcvspace = MSS*MAX_CWND;

    optlen = sizeof(rcvspace);
    int on = 1;
    for(k=0; k < csk->substreams; k++){
      setsockopt(csk->sockfd[k],SOL_SOCKET,SO_RCVBUF, (char *) &rcvspace, optlen);
      getsockopt(csk->sockfd[k], SOL_SOCKET, SO_RCVBUF, (char *) &rlth, (socklen_t*)&optlen);
      setsockopt(csk->sockfd[k], SOL_SOCKET, SO_TIMESTAMP, (const char *)&on, sizeof(on)); 
    }
    //printf("ctcpcli using port %s rcvspace %d\n", port,rlth);

    // ------------  Send a SYN packet for any new connection ----------------
    int path_ix = -1;
    flag_t flag;
    double msgtstamp;
    rv = 0;
    do{
      if (path_ix == -1){
        for (k = 0; k < csk->substreams; k++){
          if(send_flag(csk, k, SYN, getTime()) == -1){
            printf("Could not send SYN packet \n");
          }else{
            csk->pathstate[k] = SYN_SENT;
            csk->lastrcvt[k] = getTime();
            log_cli_status(csk);
          }
        }
        //rv++;
        rv = 1; // don't backoff timeout as losing SYN can cause long delay in connection startup 
      }
      flag = SYN_ACK;
    }while( (path_ix = poll_flag(csk, &flag, (2<<(rv-1))*POLL_ACK_TO, &msgtstamp)) < 0 && rv < POLL_MAX_TRIES );
    // poll backing off the timeout value (above) by rv*POLL_ACK_TO
    

    if(rv >= POLL_MAX_TRIES){
      //printf("Did not receive SYN ACK\n");
      return NULL;
    }else{
      //printf("Received SYN ACK after %d tries\n", rv);
      csk->pathstate[path_ix] = SYN_ACK_RECV;
      log_cli_status(csk);
      csk->lastrcvt[path_ix] = getTime();
      if(send_flag(csk, path_ix, NORMAL, msgtstamp) == 0){
        csk->pathstate[path_ix] = ESTABLISHED;
        log_cli_status(csk);
        //printf("Sent ACK for the SYN ACK\n");
      }
    }

    ///////////////////////   CONNECTION SETUP UP TO HERE  ///////////////

    // Let another thread do the job and return the socket

    rv = pthread_create( &(csk->daemon_thread), NULL, handle_connection, (void *) csk);
    return csk;
}

uint32_t 
read_ctcp(clictcp_sock* csk, void *usr_buf, size_t count){

  if (csk->status != ACTIVE){
    //printf("read_ctcp: not active\n");
    csk->error = SRVHUP;
    return -1;
  }

  size_t res = fifo_pop(&(csk->usr_cache), usr_buf, count);
  //printf("read_ctcp: pop %d bytes, csk->usr_cache size %d\n", res, csk->usr_cache.size);

  if (res == 0){
    // printf("read_ctcp: nothing to read\n");
    // fifo is released because the connection is somehow terminated
    csk->error = SRVHUP;
    res = -1;
  }

  log_cli_status(csk);
  return res;
}

void SIGINT_handler(void){
  //printf("***GOT SIGINT***\n");
}


void
*handle_connection(void* arg){

  clictcp_sock* csk = (clictcp_sock*) arg;
  socklen_t srvlen = sizeof(csk->srv_addr);
  int k, numbytes;//[MAX_SUBSTREAMS];
  struct timeval time_now;
  struct iovec iov;
  struct msghdr msg_sock;
  char ctrl_buff[CMSG_SPACE(sizeof(struct timeval))];
  struct cmsghdr *cmsg;
  iov.iov_len = MSS;
  msg_sock.msg_iov = &iov;
  msg_sock.msg_iovlen = 1;
  msg_sock.msg_name = &(csk->srv_addr);
  msg_sock.msg_namelen = srvlen;
  msg_sock.msg_control = (caddr_t)ctrl_buff;
  msg_sock.msg_controllen = sizeof(ctrl_buff);
  Skb* skb;
 
  // READING FROM MULTIPLE SOCKET
  struct pollfd read_set[MAX_SUBSTREAMS];

  for(k=0; k < csk->substreams; k++){
    read_set[k].fd = csk->sockfd[k];
    read_set[k].events = POLLIN;
  }

  double idle_timer;
  int curr_substream=0;
  int ready, i;
  int poll_rv, tries;
  flag_t flag;

  // prepare the signal handler for SIGINT
  struct sigaction sa;

  sa.sa_handler = (void *) SIGINT_handler; //SIG_IGN;
  sa.sa_flags = 0; 
  sigemptyset(&sa.sa_mask);
  
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
  }


  do{
    idle_timer = getTime();
    double msgtstamp = idle_timer;

    // value -1 blocks until something is ready to read
    // Replace ppoll() call with portable equivalent
    //ready = ppoll(read_set, csk->substreams, NULL, &(sa.sa_mask));
    sigset_t origmask;
    sigprocmask(SIG_SETMASK, &(sa.sa_mask), &origmask);
    ready = poll(read_set, csk->substreams, -1);
    sigprocmask(SIG_SETMASK, &origmask, NULL);

    if(ready == -1){
      if (errno == EINTR && csk->status == CLOSED){
        break;   // this should free and exit this thread
      } else{
        perror("poll");
      }
    }else if (ready == 0){
      // printf("Timeout occurred during poll! Should not happen with -1\n");
      //ready = POLL_TO_FLG;
    }else{
      srvlen = sizeof(csk->srv_addr); 
      do{
        skb = alloc_skb(csk->debug);
        Msgbuf* msgbuf = &(skb->msgbuf);
        iov.iov_base = msgbuf->buff;
        Data_Pckt* msg = &(msgbuf->msg);
        if(read_set[curr_substream].revents & POLLIN){
          if((numbytes = recvmsg(csk->sockfd[curr_substream], &msg_sock, 0))== -1){
            err_sys("recvmsg",csk);

            // removing path if it returns -1 on recvfrom (doesn't exist anymore)
            csk->pathstate[curr_substream] = CLOSING;
            log_cli_status(csk);
            // update read_set when removing a substream
            for(i=curr_substream; i < csk->substreams-1; i++){
              read_set[i].fd = read_set[i+1].fd;
              read_set[i].events = POLLIN;
            }
            read_set[csk->substreams-1].fd = 0;

            remove_substream(csk, curr_substream);
            curr_substream--;
            
            log_cli_status(csk);
          }
          if(numbytes <= 0) break;
          // use packet timestamp from kernel, if possible ...
          cmsg = CMSG_FIRSTHDR(&msg_sock);
          if (cmsg &&
             cmsg->cmsg_level == SOL_SOCKET &&
             cmsg->cmsg_type == SCM_TIMESTAMP &&
             cmsg->cmsg_len == CMSG_LEN(sizeof(time_now))) {
                               /* Copy to avoid alignment problems: */
             memcpy(&time_now,CMSG_DATA(cmsg),sizeof(time_now));
             csk->lastrcvt[curr_substream] = time_now.tv_sec + time_now.tv_usec*1e-6;
          } else {
             csk->lastrcvt[curr_substream] = getTime();
          }
          csk->idle_time[curr_substream] = INIT_IDLE_TIME;
          csk->idle_count[curr_substream] = 0;

          csk->idle_total += csk->lastrcvt[curr_substream] - idle_timer;
          csk->pkts++;
          if (csk->start_time == 0) csk->start_time =  csk->lastrcvt[curr_substream];   /* first pkt time */

          // Unmarshall the packet
          bool match = unmarshallData(msgbuf, csk);

          switch (msg->flag){

          case FIN:
            // printf("received FIN packet %u\n", getpid());
            csk->pathstate[curr_substream] = FIN_RECV;
            log_cli_status(csk);
            tries = 0;
            poll_rv = -1;

            do{
              if (poll_rv == -1){
                for(i=0; i<csk->substreams; i++){
                  send_flag(csk, i, FIN_ACK,msg->tstamp);
                  csk->pathstate[i] = FIN_ACK_SENT;
                  log_cli_status(csk);
                }
                tries++;
              }

              flag = FIN_ACK;
              msgtstamp = msg->tstamp;
              poll_rv = poll_flag(csk, &flag, POLL_ACK_TO*(2<<(tries-1)),&msgtstamp);
            }while ( poll_rv < 0  && tries < POLL_MAX_TRIES);          

            if(tries >= POLL_MAX_TRIES){
              //printf("Did not receive FIN_ACK_ACK... Closing anyway\n");
            }else{
              //printf("Received FIN_ACK_ACK after %d tries\n", tries);
              for(i=0; i<csk->substreams; i++){
                csk->pathstate[i] = CLOSING;
                log_cli_status(csk);
                //remove_substream(csk, i);                
              }
            }
            csk->status = CLOSED;
            log_cli_status(csk);
            fifo_release(&(csk->usr_cache));            
            ready = 0;
            break;

          case FIN_ACK:
            // We should never really be here, since this should happen in close_clictcp
            // unless we want to close and open individual paths independently later

            if(csk->pathstate[curr_substream] == FIN_SENT ||
               csk->pathstate[curr_substream] == FIN_ACK_RECV){

              //printf("received FIN_ACK packet\n");
              csk->pathstate[curr_substream] = FIN_ACK_RECV;
              log_cli_status(csk);
              for(i = 0; i<csk->substreams; i++){
                send_flag(csk, i, FIN_ACK, msg->tstamp);
                csk->pathstate[i] = CLOSING;
                log_cli_status(csk);
                //remove_substream(csk, i);
              }
              csk->status = CLOSED;
              log_cli_status(csk);
              fifo_release(&(csk->usr_cache));      
            }else{
              //printf("State %d: received FIN_ACK\n", csk->pathstate[curr_substream]);
            }
            break;
          
          case SYN_ACK:
            if(csk->pathstate[curr_substream] == SYN_SENT ||
               csk->pathstate[curr_substream] == SYN_ACK_RECV ||
               csk->pathstate[curr_substream] == ESTABLISHED){
              
              // if(csk->pathstate[curr_substream] != ESTABLISHED){
              //  csk->pathstate[curr_substream] = SYN_ACK_RECV;
              //  }
              
              if (send_flag(csk, curr_substream, NORMAL,msg->tstamp) == 0){
                csk->pathstate[curr_substream] = ESTABLISHED;
                log_cli_status(csk);
              }

            }else{
              //printf("State %d: Received SYN_ACK\n", csk->pathstate[curr_substream]);
            }
            break;

          case SYN:
            printf("ERROR: received SYN on the client side\n");
            break;

          default:
            if (csk->debug > 6){
              printf("seqno %d num pkts %d start pkt %d curr_block %d dofs %d\n",
                     msg->seqno,  msg->num_packets, msg->start_packet, 
                     csk->curr_block, csk->blocks[csk->curr_block%NUM_BLOCKS].dofs);
            }
            if (csk->debug > 6 && msg->blockno != csk->curr_block ){
              printf("exp %d got %d\n", csk->curr_block, msg->blockno);
            }
            
            if(csk->pathstate[curr_substream] == ESTABLISHED){
              bldack(csk, skb, curr_substream);            
            }else if(csk->pathstate[curr_substream] == SYN_ACK_RECV ||
                     csk->pathstate[curr_substream] == SYN_SENT){
              csk->pathstate[curr_substream] = ESTABLISHED;
              log_cli_status(csk);
              bldack(csk, skb, curr_substream);            
            }else{
              //printf("State %d: Received a data packet\n", csk->pathstate[curr_substream]);
            }
          }
           
          ready--;    // decrease the number of ready sockets
        }
        curr_substream++;
        if(curr_substream == csk->substreams) curr_substream = 0;

      }while(ready>0);
    }
      
    if (csk->status != CLOSED){
      for(k = 0; k<csk->substreams; k++){
        if(getTime() - csk->lastrcvt[k] > csk->idle_time[k]){     
          if(csk->pathstate[k] == ESTABLISHED){
            continue;
          }else if(csk->pathstate[k] == SYN_SENT){
            send_flag(csk, k, SYN,getTime());
          }else if(csk->pathstate[k] == SYN_ACK_RECV){
            send_flag(csk, k, NORMAL,msgtstamp);
          }else if(csk->pathstate[k] == FIN_SENT){
            send_flag(csk, k, FIN,getTime());
          }else if(csk->pathstate[k] == FIN_RECV ||
                   csk->pathstate[k] == FIN_ACK_SENT){
            send_flag(csk, k, FIN_ACK,msgtstamp);
          }else if(csk->pathstate[k] == FIN_ACK_RECV){
            send_flag(csk, k, FIN_ACK,msgtstamp);
            csk->pathstate[k] = CLOSING;
            log_cli_status(csk);

            // update read_set when removing a substream
            for(i=k; i < csk->substreams-1; i++){
              read_set[i].fd = read_set[i+1].fd;
              read_set[i].events = POLLIN;
            }
            read_set[csk->substreams-1].fd = 0;

            remove_substream(csk, k);
            k--;

          }// otherwise, CLOSING
          csk->idle_time[k] = 2*csk->idle_time[k];
          csk->idle_count[k]++;
          if (csk->idle_count[k] >= IDLE_MAX_COUNT){
            //printf("\nMAX IDLE on path %d\n", k);
            if(csk->pathstate[k] == SYN_ACK_RECV ||
               csk->pathstate[k] == ESTABLISHED){
              send_flag(csk, k, FIN,getTime());         // TODO change it to FIN_PATH so the server doesn't close
              csk->pathstate[k] = FIN_SENT;
              log_cli_status(csk);
            }else{
              // Tried enough, should close now
              //printf("\nTried enough on path %d.\n", k);
              csk->pathstate[k] = CLOSING;
              log_cli_status(csk);

              // update read_set when removing a substream
              for(i=k; i < csk->substreams-1; i++){
                read_set[i].fd = read_set[i+1].fd;
                read_set[i].events = POLLIN;
              }
              read_set[csk->substreams-1].fd = 0;

              remove_substream(csk, k);
            }
          }
        }
      }
    }    

    if(csk->substreams == 0){
      csk->status = CLOSED;
      log_cli_status(csk);
      fifo_release(&(csk->usr_cache));      
    }
  }while(csk->status == ACTIVE); 
  pthread_exit(NULL);
  
}

void
remove_substream(clictcp_sock* csk, int pin){
  if(csk->pathstate[pin] != CLOSING){
    printf("ERROR: Path %d is not in state CLOSING\n", pin);
  }else{
    //printf("removing path %d\n", pin);
    int i;
    for(i = pin; i<csk->substreams; i++){
      csk->ifc_addr[i] = csk->ifc_addr[i+1];
      csk->sockfd[i] = csk->sockfd[i+1];
      csk->pathstate[i] = csk->pathstate[i+1];
      csk->lastrcvt[i] = csk->lastrcvt[i+1];
      csk->idle_time[i] = csk->idle_time[i+1];   
      csk->idle_count[i] = csk->idle_count[i+1];
    }
    csk->substreams--;
    
    csk->ifc_addr[csk->substreams] = NULL;
  }
}


void
err_sys(char *s, clictcp_sock *csk){
    perror(s);
    ctrlc(csk);     /* do stats on error */
}

void
bldack(clictcp_sock* csk, Skb* skb, int curr_substream){
  socklen_t srvlen;
  double elimination_timer = getTime();
  Data_Pckt* msg = &(skb->msgbuf.msg);
  uint32_t blockno = msg->blockno;    //The block number of incoming packet
  uint8_t start;
  uint8_t coeff[BLOCK_SIZE];
  bool save_skb=FALSE;

  csk->end_time = getTime();  /* last read */

  if (msg->seqno > csk->last_seqno+1){
    //printf("Loss report blockno %d Number of losses %d\n", msg->blockno, msg->seqno - last_seqno - 1);
    csk->total_loss += msg->seqno - (csk->last_seqno+1);
  }

    csk->last_seqno = MAX(msg->seqno, csk->last_seqno) ;  // ignore out of order packets

    if (blockno < csk->curr_block){
        // Discard the packet if it is coming from a decoded block or it is too far ahead
        // Send an appropriate ack to return the token
      if (csk->debug > 5){
        printf("Old packet  curr block %d packet blockno %d seqno %d substream %d \n", 
               csk->curr_block, blockno, msg->seqno, curr_substream);
      }
      csk->old_blk_pkts++;
    }else if (blockno >= csk->curr_block + NUM_BLOCKS){
      printf("BAD packet: The block %u, %u does not exist yet.\n", blockno, msg->seqno);
    }else{
        // Otherwise, the packet should go to one of the blocks in the memory
        // perform the Gaussian elimination on the packet and put in proper block

        int prev_dofs = csk->blocks[blockno%NUM_BLOCKS].dofs;

        // If the block is already full, skip Gaussian elimination
        if(csk->blocks[blockno%NUM_BLOCKS].dofs < csk->blocks[blockno%NUM_BLOCKS].len){

          start = msg->start_packet;
          if (msg->flag == CODED) {
            // reconstruct random linear coefficients for coded packets
            seedfastrand((uint32_t) (msg->packet_coeff+blockno));
            for (int i=0; i<msg->num_packets; i++) coeff[i]= (uint8_t) (fastrand()%GF);
          } else 
            coeff[0]=1;
          
          // Shift the row to make SHURE the leading coefficient is not zero
          int shift = shift_row(coeff, msg->num_packets);
          start += shift;
          
          // THE while loop!
          while(!isEmpty(coeff, msg->num_packets)){
            if(csk->blocks[blockno%NUM_BLOCKS].rows[start][0] == -1){
              msg->num_packets = MIN(msg->num_packets, BLOCK_SIZE - start);
              
              csk->blocks[blockno%NUM_BLOCKS].row_len[start] = msg->num_packets;              
              // Set the coefficients to be all zeroes (for padding if necessary)
              memset(csk->blocks[blockno%NUM_BLOCKS].rows[start], 0, msg->num_packets);
              // Normalize the coefficients and the packet contents
              if (GF==256) normalize(coeff, msg->payload, msg->num_packets);
              // Put the coefficients into the matrix
              memcpy(csk->blocks[blockno%NUM_BLOCKS].rows[start], coeff, msg->num_packets);
              // Put the payload into the corresponding place
              //memcpy(csk->blocks[blockno%NUM_BLOCKS].content[start], msg->payload, PAYLOAD_SIZE);
              csk->blocks[blockno%NUM_BLOCKS].content[start] = msg->payload;
              // and keep a pointer to the original skb, so we can release it later
              csk->blocks[blockno%NUM_BLOCKS].skb[start] = skb;
              save_skb = TRUE;

              if (csk->blocks[blockno%NUM_BLOCKS].rows[start][0] != 1){
                printf("blockno %d\n", blockno);
              }

              // We got an innovative eqn
              csk->blocks[blockno%NUM_BLOCKS].max_coding_wnd = 
                MAX(csk->blocks[blockno%NUM_BLOCKS].max_coding_wnd, msg->num_packets);
              csk->blocks[blockno%NUM_BLOCKS].dofs++;
              break;
            }else{
              uint8_t pivot = coeff[0];
              int i;

              if (csk->debug > 9){
                int ix;
                for (ix = 0; ix < msg->num_packets; ix++){
                  printf(" %d ", coeff[ix]);
                }
                printf("seqno %d start%d isEmpty %d \n Row coeff", 
                       msg->seqno, start, isEmpty(coeff, msg->num_packets)==1);

                for (ix = 0; ix < msg->num_packets; ix++){
                  printf(" %d ",csk->blocks[blockno%NUM_BLOCKS].rows[start][ix]);
                }
                printf("\n");
              }

              coeff[0] = 0; // TODO check again
              // Subtract row with index start with the row at hand (coffecients)
              if (GF==256) {
                 uint8_t logpivot = xFFlog(pivot);
                 for(i = 1; i < csk->blocks[blockno%NUM_BLOCKS].row_len[start]; i++){
                   coeff[i] ^= fastFFmult(csk->blocks[blockno%NUM_BLOCKS].rows[start][i], logpivot);
                 }

                 // Subtract row with index start with the row at hand (content)
                 for(i = 0; i < PAYLOAD_SIZE; i++){
                   msg->payload[i] ^= fastFFmult(csk->blocks[blockno%NUM_BLOCKS].content[start][i], logpivot);
                 }
              } else {
                 // GF(2)
                 for(i = 1; i < csk->blocks[blockno%NUM_BLOCKS].row_len[start]; i++){
                   coeff[i] ^= csk->blocks[blockno%NUM_BLOCKS].rows[start][i];
                 }
                 for(i = 0; i < PAYLOAD_SIZE; i+=4){
                    msg->payload[i] ^= csk->blocks[blockno%NUM_BLOCKS].content[start][i];
                    msg->payload[i+1] ^= csk->blocks[blockno%NUM_BLOCKS].content[start][i+1];
                    msg->payload[i+2] ^= csk->blocks[blockno%NUM_BLOCKS].content[start][i+2];
                    msg->payload[i+3] ^= csk->blocks[blockno%NUM_BLOCKS].content[start][i+3];
                 }
              }

              // Shift the row
              shift = shift_row(coeff, msg->num_packets);
              start += shift;
            }
          } // end while

          if(csk->blocks[blockno%NUM_BLOCKS].dofs == prev_dofs){
            csk->ndofs++;
          }
        } else {  // end if block.dof < block.len
          csk->nxt_blk_pkts++;   // If the block is full rank but not yet decoded, anything arriving is old
          if (csk->debug > 5){
            printf("NEXT packet  curr block %d packet blockno %d seqno %d substream %d\n", 
                   csk->curr_block, blockno, msg->seqno, curr_substream);
          }
        }

        csk->elimination_delay += getTime() - elimination_timer;

        //printf("current blk %d\t dofs %d \n ", curr_block, blocks[curr_block%NUM_BLOCKS].dofs);

    } // end else (if   curr_block <= blockno <= curr_block + NUM_BLOCKS -1 )

    //------------------------------------------------------------------------------------------------------

    // Build the ack packet according to the new information
    Skb* ackskb = ackPacket(msg->seqno+1, csk->curr_block,
                              csk->blocks[csk->curr_block%NUM_BLOCKS].dofs, csk->debug);
    Ack_Pckt* ack = &(ackskb->msgbuf.ack);
    while( (ack->dof_rec == csk->blocks[(ack->blockno)%NUM_BLOCKS].len) && 
           (ack->blockno < csk->curr_block + NUM_BLOCKS)  ){
        // The current block is decodable, so need to request for the next block
        // XXX make sure that NUM_BLOCKS is not 1, or this will break
        ack->blockno++;
        ack->dof_rec = csk->blocks[(ack->blockno)%NUM_BLOCKS].dofs;
    }

    if (ack->blockno >= csk->curr_block + NUM_BLOCKS){
      ack->dof_rec = 0;
    }
    // compensate for processing time between when packet arrived and when send ack ...
    ack->tstamp = msg->tstamp + getTime()-csk->lastrcvt[curr_substream];

    // Marshall the ack into buff
    int size = marshallAck(&(ackskb->msgbuf));
    char* buff = (char*) &(ackskb->msgbuf.buff);
    srvlen = sizeof(csk->srv_addr);

    send_over(csk, curr_substream, buff, size);

    csk->acks++;

    free_skb(ackskb);

    if (csk->debug > 6){
        printf("Sent an ACK: ackno %d blockno %d\n", ack->ackno, ack->blockno);
    }

    //----------------------------------------------------------------
    //      CHECK IF ANYTHING CAN BE PUSHED TO THE APPLICATION     //

    if (csk->blocks[csk->curr_block%NUM_BLOCKS].dofs < csk->blocks[csk->curr_block%NUM_BLOCKS].len){
      partial_write(csk);
    }
    
    // Always try decoding the curr_block first, even if the next block is decodable, it is not useful
    while(csk->blocks[csk->curr_block%NUM_BLOCKS].dofs == csk->blocks[csk->curr_block%NUM_BLOCKS].len){
        // We have enough dofs to decode, DECODE!

        double decoding_timer = getTime();
        if (csk->debug > 4){
            printf("Starting to decode block %d ... ", csk->curr_block);
        }

        unwrap(&(csk->blocks[csk->curr_block%NUM_BLOCKS]));

        // Write the decoded packets into the file
        writeAndFreeBlock(&(csk->blocks[csk->curr_block%NUM_BLOCKS]), &(csk->usr_cache));

        // Initialize the block for next time
        initCodedBlock(&(csk->blocks[csk->curr_block%NUM_BLOCKS]));

        // Increment the current block number
        csk->curr_block++;

        csk->decoding_delay += getTime() - decoding_timer;
        if (csk->debug > 4){
          printf("Done within %f secs   blockno %d max_pkt_ix %d \n", 
                 getTime()-decoding_timer, 
                 csk->curr_block, csk->blocks[(csk->curr_block)%NUM_BLOCKS].max_packet_index);
        }
    } // end if the block is done
    if (!save_skb) free_skb(skb);

}
//========================== END Build Ack ===============================================================


void
partial_write(clictcp_sock* csk){

  int blockno = csk->curr_block;
  uint8_t start = csk->blocks[blockno%NUM_BLOCKS].dofs_pushed;
  bool push_ready = TRUE; 
  uint16_t payload_len;
  int i;
  size_t bytes_pushed, push_tmp;

  if(csk->blocks[blockno%NUM_BLOCKS].dofs == csk->blocks[blockno%NUM_BLOCKS].max_packet_index){
    // We have enough dofs to decode, DECODE!
    unwrap(&(csk->blocks[blockno%NUM_BLOCKS]));
 }
  do {
    if ( csk->blocks[blockno%NUM_BLOCKS].rows[start][0] == -1){
      push_ready = FALSE;
    } else {
      for (i = 1; i < csk->blocks[blockno%NUM_BLOCKS].row_len[start]; i++){
        if (csk->blocks[blockno%NUM_BLOCKS].rows[start][i]){
          push_ready = FALSE;
          break;
        }
      }
    }

    if (push_ready){
      // check the queue size
      // if enough room, push, otherwise, exit the push process
      // Read the first two bytes containing the length of the useful data
      memcpy(&payload_len, csk->blocks[blockno%NUM_BLOCKS].content[start], 2);
      
      // Convert to host order
      payload_len = ntohs(payload_len);
      if (fifo_getspace(&(csk->usr_cache)) >= payload_len){
        // push the packet to user cache
                
        // Write the contents of the decode block into the file
        //fwrite(blocks[blockno%NUM_BLOCKS].content[i]+2, 1, len, rcv_file);
        
        bytes_pushed = 0;
        while (bytes_pushed < payload_len){
          push_tmp = fifo_push(&(csk->usr_cache), 
                                    csk->blocks[blockno%NUM_BLOCKS].content[start]+2+bytes_pushed, 
                                    payload_len - bytes_pushed);
          if (push_tmp == 0){
            return; //fifo has been released
          }

          bytes_pushed += push_tmp;
        }

        //printf("write_ctcp: pushed %d bytes blockno %d max_pkt_ix %d start %d\n", 
        //       bytes_pushed, blockno, csk->blocks[blockno%NUM_BLOCKS].max_packet_index, start);
        //printf("blockno %d dofs %d max_pkt_ix %d dofs_pushed %d payload_len %d\n", blockno, csk->blocks[blockno%NUM_BLOCKS].dofs, csk->blocks[blockno%NUM_BLOCKS].max_packet_index, start, payload_len);

        start++;

      }else{
        push_ready = FALSE;
      }
    }

  } while (push_ready);

  csk->blocks[blockno%NUM_BLOCKS].dofs_pushed = start;

  return;
}

void
normalize(uint8_t* coefficients, char*  payload, uint8_t size){
  if (coefficients[0] != 1){
    uint8_t pivot = inv_vec[coefficients[0]];
    int i;
    uint8_t logpivot=xFFlog(pivot);
    for(i = 0; i < size; i++){
      coefficients[i] = fastFFmult(coefficients[i], logpivot);
    }
    
    for(i = 0; i < PAYLOAD_SIZE; i++){
      payload[i] = fastFFmult(payload[i],logpivot);
    }
  }
}


int
shift_row(uint8_t* buf, int len){
    if(len == 0) return 0;

    // Get to the first nonzero element
    int shift;
    for(shift=0; shift < len; shift++){
      if (buf[shift] != 0) break;
    }

    if(shift == 0) return shift;

    int i;
    for(i = 0; i < len-shift; i++){
        buf[i] = buf[shift+i];
    }

    memset(buf+len-shift, 0, shift);
    return shift;
}

bool
isEmpty(uint8_t* coefficients, uint8_t size){
    uint8_t result = 0;
    int i;
    for(i = 0; i < size; i++) result |= coefficients[i];
    return (result == 0);
}

/*
 * Initialize a new block struct
 */
void
initCodedBlock(Coded_Block_t *blk){
    blk->dofs = 0;
    blk->len  = BLOCK_SIZE; // this may change once we get packets
    blk->max_coding_wnd = 0;
    blk->dofs_pushed = 0;
    blk->max_packet_index = 0;

    int i;
    for(i = 0; i < BLOCK_SIZE; i++){
        blk->rows[i][0]    = -1;
        blk->row_len[i]    = 0;
        blk->content[i] = NULL;
    }
}

void
unwrap(Coded_Block_t *blk){
    int row;
    int offset;
    int byte;
    uint8_t log;
    //prettyPrint(blocks[blockno%NUM_BLOCKS].rows, MAX_CODING_WND);
    for(row = blk->max_packet_index-2; row >= blk->dofs_pushed; row--){
      
      /*
        int k;
        printf("row[%d] = ", row);
        for (k = 0; k < blk->row_len[row]; k++){
          printf("%d, ", blk->rows[row][k]);
        }
        printf("\n");
      */

        for(offset = 1; offset <  blk->row_len[row]; offset++){
            if(blk->rows[row][offset] == 0)
                continue;
            if (GF==256) {
               log = xFFlog(blk->rows[row][offset]);
               for(byte = 0; byte < PAYLOAD_SIZE; byte++){
                   blk->content[row][byte]
                       ^= fastFFmult(blk->content[row+offset][byte], log );
               }
            } else if (blk->rows[row][offset]>0) {
               // GF(2)
               for(byte = 0; byte < PAYLOAD_SIZE; byte++){
                  blk->content[row][byte] ^= blk->content[row+offset][byte];
               }
            }
        }

        blk->row_len[row] = 1;   // Now this row has only the diagonal entry
    }
}


void
writeAndFreeBlock(Coded_Block_t *blk, fifo_t *buffer){
    uint16_t len;
    int push_tmp, bytes_pushed, i;
    //printf("Writing a block of length %d\n", blocks[blockno%NUM_BLOCKS].len);

    for(i = blk->dofs_pushed; i < blk->len; i++){
        // Read the first two bytes containing the length of the useful data
        memcpy(&len, blk->content[i], 2);

        // Convert to host order
        len = ntohs(len);
        // Write the contents of the decode block into the file
        //fwrite(blocks[blockno%NUM_BLOCKS].content[i]+2, 1, len, rcv_file);

        bytes_pushed = 0;
        while (bytes_pushed < len){
          push_tmp = fifo_push(buffer, blk->content[i]+2+bytes_pushed, len - bytes_pushed);

          if (push_tmp == 0){
            return; //fifo has been released
          }

          bytes_pushed += push_tmp;
        }

    }

    for(i = 0; i < blk->len; i++){
       free_skb(blk->skb[i]);
    }

}

bool
unmarshallData(Msgbuf *msgbuf, clictcp_sock *csk){
    
    ntohpData(&(msgbuf->msg));
    // Need to make sure all the incoming packet_coeff are non-zero
    if (msgbuf->msg.blockno >= csk->curr_block){
      csk->blocks[msgbuf->msg.blockno%NUM_BLOCKS].max_packet_index = MAX(msgbuf->msg.start_packet + msgbuf->msg.num_packets , csk->blocks[msgbuf->msg.blockno%NUM_BLOCKS].max_packet_index);
    }
    return TRUE;
}


int
marshallAck(Msgbuf* msgbuf){
    htonpAck(&(msgbuf->ack));
    return ACK_SIZE;
}

int
readLease(char *leasefile, dhcp_lease *leases){
  
  /* read lease file if there, keyword value */
  FILE *fp;
  char line[128], type[64], val1[64], val2[64];
  int substreams = -1;

  fp = fopen(leasefile,"r");

  if (fp == NULL) {
    printf("ctcp unable to open %s\n",leasefile);
    return -1;
  }

  while (fgets(line, sizeof (line), fp) != NULL) {
    sscanf(line,"%s %s %s",type, val1, val2);
    if (*type == '#') continue;
    else if (strcmp(type,"lease")==0){
      substreams++;
      
      //printf("lease %d\n", substreams);
    }
    else if (strcmp(type,"fixed-address")==0){
      leases[substreams].address = strndup(val1, strlen(val1)-1);
      //printf("Address: %s\n", leases[substreams].address);
    }    
    else if (strcmp(type,"interface")==0){
      leases[substreams].interface = strndup(val1+sizeof(char), strlen(val1)-3); 
      //printf("Iface: %s\n", leases[substreams].interface);
    }    
    else if (strcmp(type,"option")==0){
      if (strcmp(val1,"subnet-mask")==0){
        leases[substreams].netmask = strndup(val2, strlen(val2)-1);
        //printf("Netmask: %s\n", leases[substreams].netmask);
      }    
      else if (strcmp(val1,"routers")==0){
        leases[substreams].gateway = strndup(val2, strlen(val2)-1);
        //printf("Gateway:%s\n\n", leases[substreams].gateway);
      }    
    }
    
  }

  return substreams+1; 
}

int 
add_routing_tables(char *lease_file){
  
  if (lease_file == NULL){
    return 0;   // Do nothing
  }

  int k, substreams;
  dhcp_lease leases[MAX_SUBSTREAMS];

  substreams = readLease(lease_file, leases);

  for (k=0; k < substreams; k++){
    make_new_table(&leases[k], k+1, k+1);
  }

  return substreams;
}

void 
remove_routing_tables(char *lease_file){

  if (lease_file == NULL){
    return;   // Do nothing
  }

  int k, substreams;
  dhcp_lease leases[MAX_SUBSTREAMS];

  substreams = readLease(lease_file, leases);

  for (k=0; k < substreams; k++){
    delete_table(k+1, k+1);      // Flush the routing tables and iptables
  }

  return;
}




void 
make_new_table(dhcp_lease* lease, int table_number, int mark_number){

  printf("/****** making table %d, mark %d ****** Interface: %s  ******** IP address: %s *****/\n", table_number, mark_number, lease->interface, lease->address );

  // command to be used for system()
  char* command = malloc(150);

  // Flush routing table (table_number) //
  sprintf(command, "ip route flush table %d", table_number);
  //printf("%s\n", command);
  system(command);
  
  // Figure out the Network Address, Netmask Number, etc.//
  struct in_addr* address = malloc(sizeof(struct in_addr));
  struct in_addr* netmask = malloc(sizeof(struct in_addr));
  struct in_addr* network_addr = malloc(sizeof(struct in_addr)); //Network address
  uint32_t mask, network;
  int maskbits; //Netmask number
  if(!inet_aton(lease->address, address)){
    printf("%s is not a good IP address\n", lease->address);
    exit(1);
  }
  if(!inet_aton(lease->netmask, netmask)){
    printf("%s is not a good netmask\n", lease->netmask);
    exit(1);
  }
  /* compute netmask number*/
  mask = ntohl(netmask->s_addr);
  for(maskbits=32; (mask & 1L<<(32-maskbits))==0; maskbits--);
  

  // printf("IP address %s\n", inet_ntoa(*address));
  // printf("Netmask %s\n", inet_ntoa(*netmask));
  // printf("Netmask bits %d\n", maskbits);
  
  /* compute network address -- AND netmask and IP addr */
  network = ntohl(address->s_addr) & ntohl(netmask->s_addr);
  network_addr->s_addr = htonl(network);
  // printf("Network %s\n", inet_ntoa(*network_addr));

  // Add routes to the routing table (table_number)//
  memset(command, '\0', sizeof(command));
  sprintf(command, "ip route add table %d %s/%d dev %s proto static src %s", table_number, inet_ntoa(*network_addr), maskbits, lease->interface, lease->address);
  system(command);

  memset(command, '\0', sizeof(command));
  sprintf(command, "ip route add table %d default via %s dev %s proto static", table_number, lease->gateway, lease->interface);
  system(command);
  
  //memset(command, '\0', sizeof(command));
  //sprintf(command, "ip route show table %d", table_number);
  //printf("%s\n", command);
  //system(command);
  //printf("\n");

  // Create and add rules//
  memset(command, '\0', sizeof(command));
  sprintf(command, "iptables -t mangle -A OUTPUT -s %s -j MARK --set-mark %d", lease->address, mark_number);
  system(command);

  memset(command, '\0', sizeof(command));
  sprintf(command, "ip rule add fwmark %d table %d", mark_number, table_number);
  system(command);
  
  //system("iptables -t mangle -S");
  //printf("\n");

  //printf("ip rule show\n");
  //system("ip rule show");
  //printf("\n");
  
  //printf("ip route flush cache\n");
  system("ip route flush cache");
  //printf("\n");
  
  return;
}

void
delete_table(int table_number, int mark_number){
  printf("/****** deleting table %d, mark %d *****/\n", table_number, mark_number);
  char* command = malloc(150);

  // Flush routing table (table_number) //
  sprintf(command, "ip route flush table %d", table_number);
  system(command);

  memset(command, '\0', sizeof(command));
  sprintf(command, "ip rule delete fwmark %d table %d", mark_number, table_number);
  if (system(command) == -1){
    printf("No ip rule to delete\n");
  }

  memset(command, '\0', sizeof(command));
  sprintf(command, "iptables -t mangle -F");
  system(command);

  // Printing...
  //memset(command, '\0', sizeof(command));
  //sprintf(command, "ip route show table %d", table_number);
  //printf("%s\n", command);
  //system(command);
  //printf("\n");

  //printf("ip rule show\n");
  //system("ip rule show");
  //printf("\n");
  
  //printf("ip route flush cache\n");
  system("ip route flush cache");
  //printf("\n");
}

clictcp_sock* 
create_clictcp_sock(struct child_local_cfg* cfg){
  int k;

  clictcp_sock* sk = malloc(sizeof(clictcp_sock));

  sk-> curr_block = 1;
    
  // Initialize the blocks
  for(k = 0; k < NUM_BLOCKS; k++){
    initCodedBlock(&(sk->blocks[k]));
  }

  // MULTIPLE SUBSTREAMS
  sk->substreams = 1;
 
  for (k=0; k < MAX_SUBSTREAMS; k++){
    sk->idle_time[k] = INIT_IDLE_TIME;
    sk->idle_count[k] = 0;
    sk->ifc_addr[k] = NULL;
    sk->sockfd[k] = 0;
  }

  fifo_init(&(sk->usr_cache), NUM_BLOCKS*BLOCK_SIZE*PAYLOAD_SIZE);

  sk->status = ACTIVE;
  sk->error = NONE;

  //---------------- STATISTICS & ACCOUTING ------------------//
  sk->pkts = 0;
  sk->acks = 0;
  sk->debug = 0;
  sk->ndofs = 0;
  sk->old_blk_pkts = 0;
  sk->nxt_blk_pkts = 0;
  sk->total_loss = 0;
  sk->idle_total = 0; // The total time the client has spent waiting for a packet
  sk->decoding_delay = 0;
  sk->elimination_delay = 0;
  sk->last_seqno = 0;
  sk->start_time = 0;

  strcpy(sk->logdir, cfg->logdir); // copy settings passed from config file
  sk->ctcp_probe = cfg->ctcp_probe;
  sk->debug = cfg->debug;
    
  return sk;
}


int
send_flag(clictcp_sock *csk, int path_id, flag_t flag, double tstamp){

  int numbytes;
  Skb* ackskb = ackPacket(0,0,0,csk->debug);
  Ack_Pckt* msg = &(ackskb->msgbuf.ack);
  msg->tstamp = tstamp;
  msg->flag = flag;

  int size = marshallAck(&(ackskb->msgbuf));
  char* buff = (char*) &(ackskb->msgbuf.buff);

  if (send_over(csk, path_id, buff, size) == -1){
    perror("send_flag error: sendto");
    free_skb(ackskb);
    return -1;
  }
  
  //  printf("Sent flag *** %d *** %u\n", flag, getpid());

  free_skb(ackskb);
  return 0;
}


int 
poll_flag(clictcp_sock *csk, flag_t* flag, int timeout, double* tstamp){
  Msgbuf msgbuf;
  int k, ready, numbytes;
  int curr_substream = 0;
  socklen_t srvlen;

  // READING FROM MULTIPLE SOCKET
  struct pollfd read_set[csk->substreams];
  for(k=0; k < csk->substreams; k++){
    read_set[k].fd = csk->sockfd[k];
    read_set[k].events = POLLIN;
  }

  // value -1 blocks until something is ready to read
  ready = poll(read_set, csk->substreams, timeout);

  if(ready == -1){
    perror("poll");
    return -1;
  }else if (ready == 0){
    // printf("Timeout occurred during poll!\n");
    return -1;
  }else{
    srvlen = sizeof(csk->srv_addr);
    while (curr_substream < csk->substreams){

      if(read_set[curr_substream].revents & POLLIN){
        if((numbytes = recvfrom(csk->sockfd[curr_substream], &(msgbuf.buff), MSS, 0,
                                &(csk->srv_addr), &srvlen)) == -1){
          err_sys("recvfrom",csk);

          // removing path if it returns -1 on recvfrom (doesn't exist anymore)
          csk->pathstate[curr_substream] = CLOSING;
          log_cli_status(csk);
          // update read_set when removing a substream
          for(k=curr_substream; k < csk->substreams-1; k++){
            read_set[k].fd = read_set[k+1].fd;
            read_set[k].events = POLLIN;
          }
          read_set[csk->substreams-1].fd = 0;
          
          remove_substream(csk, curr_substream);
          curr_substream--;
        }
        if(numbytes <= 0) {
          return -1;
        }
          
        // Unmarshall the packet
        bool match = unmarshallData(&msgbuf, csk);
        if (msgbuf.msg.flag == *flag){
          *tstamp = msgbuf.msg.tstamp;
          return curr_substream;
        }else{
          // printf("Expected flag ACK, received something else!\n");
          *flag = msgbuf.msg.flag;
          return -2;
        }
      }

      curr_substream++;
    }  /* end while */
  }
  
  //  printf("poll flag *** %d *** \n", *flag);
  return -1;

}


void
close_clictcp(clictcp_sock* csk){

  status_t sock_status_old = csk->status;

  csk->status = CLOSED;

  fifo_release(&(csk->usr_cache));      
  pthread_kill(csk->daemon_thread, SIGINT);   // send  a signal to let the thread exit properly
  pthread_join(csk->daemon_thread, NULL);

  int i;
  flag_t flag;
  double msgtstamp = getTime();
  int tries = 0;
  int index = -1;
  if (sock_status_old != CLOSED){
    do{
      if (index == -1){
        for (i =0; i<csk->substreams; i++){
          //printf("SENDING FIN %u\n", getpid());
          if(send_flag(csk, i, FIN,getTime()) == -1){
            printf("Could not send the FIN packet\n");
          }else{
            csk->pathstate[i] = FIN_SENT;
            log_cli_status(csk);
          }
        }
        tries++;      
      } else if (index == -2 && flag == FIN){
        
        for (i =0; i<csk->substreams; i++){
          //printf("SENDING FIN ACK  %u\n", getpid());
          if(send_flag(csk, i, FIN_ACK,msgtstamp) == -1){
            printf("Could not send the FIN packet\n");
          }else{
            csk->pathstate[i] = CLOSING;
            log_cli_status(csk);
          }
        }
      }

      flag = FIN_ACK;
      index = poll_flag(csk, &flag, POLL_ACK_TO*(2<<(tries-1)),&msgtstamp);
    } while( index < 0 && tries < POLL_MAX_TRIES );
    // doing multiplicative backoff for poll_flag -- may need to do this in wireless

    if(tries >= POLL_MAX_TRIES){
      // printf("Did not receive FIN ACK... Closing anyway\n");
      csk->error = CLOSE_ERR;
    }else if (csk->pathstate[index] != CLOSING){
      // printf("Received FIN ACK after %d tries\n", tries);
      csk->pathstate[index] = FIN_ACK_RECV;
      log_cli_status(csk);
      for(i = 0; i<csk->substreams; i++){
        // printf("SENDING FIN     ACK%u\n", getpid());
        if (send_flag(csk, i, FIN_ACK, msgtstamp)==0){
          csk->pathstate[i] = CLOSING;
          log_cli_status(csk);
        }
      }
    }

    csk->status = CLOSED;
    log_cli_status(csk);
  }

  // TODO free the last remaining blocks

  // close UDP sockets
  int k;
  for(k =0; k < MAX_SUBSTREAMS; k++){
    if (csk->sockfd[k] != 0){
            close(csk->sockfd[k]);
    }
  }

  log_cli_status(csk);
  if (close(csk->status_log_fd) == -1){
    perror("Could not close the status file");
  }

  // remove the status file
  char file_name[32];
  sprintf(file_name,"%s/%u",csk->logdir,getpid());

  if (remove(file_name) == -1){
    perror("Could not remove the status file");
  }

  for(k =0; k < MAX_SUBSTREAMS; k++){
    if (csk->ifc_addr[k] != NULL){
      free(csk->ifc_addr[k]);
    }
  }
  
  fifo_free(&(csk->usr_cache));  

  free(csk);

  return;
}

/*
 Send a packet over the interface associated with
 the specified substream

 returns 0 on success and -1 on failure
*/

int
send_over(clictcp_sock* csk, int substream, const void* buf, size_t buf_len){

  if (csk->ifc_addr[substream] == NULL){
    // fail over sendto
    if(sendto(csk->sockfd[0], buf, buf_len, 0, &(csk->srv_addr), (socklen_t) sizeof(csk->srv_addr)) == -1){
      perror("Sendto");
      return -1;
    }
    return 0;
  }

  struct msghdr msg;
  char ancillary_buf[CMSG_SPACE(sizeof(struct in_pktinfo))];
  struct cmsghdr *cmsg;
  struct iovec iov;

  memset(&msg, 0, sizeof(struct msghdr));
  memset(ancillary_buf, 0, sizeof(ancillary_buf));
  //memset(cmsg, 0, sizeof(struct cmsghdr));
  
  iov.iov_base = (void *)buf;
  iov.iov_len = buf_len;

  if (csk->srv_addr.sa_family != AF_INET){
    printf("SendOver: Does not support the address family of the server\n");
    return -1;
  }

  msg.msg_name = (struct sockaddr *)&(csk->srv_addr);
  msg.msg_namelen = (socklen_t) sizeof(csk->srv_addr);
  msg.msg_flags = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  
  // It gets tricky from here
  msg.msg_control = ancillary_buf;
  msg.msg_controllen = sizeof(ancillary_buf);

  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = IPPROTO_IP;
  cmsg->cmsg_type = IP_PKTINFO;
  cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));

  //  printf("Using sendmsg - substream %d \n", substream);
  struct in_pktinfo *packet_info = (struct in_pktinfo *)(CMSG_DATA(cmsg));  
  packet_info->ipi_spec_dst =  ((struct sockaddr_in*)(csk->ifc_addr[substream]))->sin_addr;
  packet_info->ipi_ifindex = 0;

  msg.msg_controllen = cmsg->cmsg_len;

  if (sendmsg(csk->sockfd[substream], &msg, 0) == -1){
    perror("Sendmsg");
    return -1;
  }

  return 0;
}

void
open_status_log(clictcp_sock* csk, char* port){

  if(!mkdir(csk->logdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
    perror("Warning making the logs directory");
  }

  char buff[128];
  sprintf(buff,"%s/%u",csk->logdir,getpid());

  csk->status_log_fd = open(buff, O_RDWR | O_CREAT | O_TRUNC,  0644);

  if (csk->status_log_fd == -1){
    //perror("Could not open status file");
    return;
  }

  
  if (flock(csk->status_log_fd, LOCK_EX) == -1){
    //perror("Could not acquire the lock for status file");
    return;
  }
  

  //FILE *f_stream fdopen(csk->status_log_fd);
 
  int len;

  len = sprintf(buff, "\nport: %s\t", port);
  write(csk->status_log_fd, buff, len);

  len = sprintf(buff, "pid: %u\t", getpid());
  write(csk->status_log_fd, buff, len);

  if (flock(csk->status_log_fd, LOCK_UN) == -1){
    perror("Could not unlock the status file");
    return;
  }

  return;
}


void
log_cli_status(clictcp_sock* csk){

  
  if (flock(csk->status_log_fd, LOCK_EX | LOCK_NB) == -1){
    perror("Could not acquire the lock for status file");
    return;
  } 

  char buff[256];
  int len;
  char sk_status_msg[2][16] = {
    "ACTIVE",
    "CLOSED"
  };

  char path_status_msg[8][16] = {
    "SYN_SENT    ",
    "SYN_ACK_RECV",
    "ESTABLISHED ",
    "FIN_SENT    ", 
    "FIN_ACK_RECV", 
    "FIN_RECV    ", 
    "FIN_ACK_SENT", 
    "CLOSING     "
  } ;


  if(lseek(csk->status_log_fd, 0, SEEK_SET) == -1){
    perror("Could not seek");
  }

  char tmp;
  int read_rv;
  int count = 0;
  do{

    read_rv = read(csk->status_log_fd, &tmp, 1);
    if (read_rv == -1){
      perror("file read");
    } else if (read_rv == 0){
      // We have reached the end of the file without success
      if (flock(csk->status_log_fd, LOCK_UN) == -1){
        perror("Could not unlock the status file");
      }
      return;
    }

    count += (tmp == '\t');
  }while(count < 2);

  // We have reached the beginning of the new line

  len = sprintf(buff, "cli_status: %s\tThru %3.2f Mbps Total loss %d Total old %d Total %d  \t", sk_status_msg[csk->status],  8.e-6*PAYLOAD_SIZE*(csk->acks)/(csk->end_time - csk->start_time + 0.1), csk->total_loss, csk->old_blk_pkts, csk->acks);
  write(csk->status_log_fd, buff, len);

  int i;
  for (i = 0; i < csk->substreams; i++){
    if (csk->ifc_addr[0] != NULL){
      len = sprintf(buff, "Path %d (%s): %s\t", i, inet_ntoa( ((struct sockaddr_in*)(csk->ifc_addr[i]))->sin_addr), path_status_msg[csk->pathstate[i]] );
    } else {
      len = sprintf(buff, "Path State: %s\t", path_status_msg[csk->pathstate[i]] );
    }

    write(csk->status_log_fd, buff, len);
  }

  if (flock(csk->status_log_fd, LOCK_UN) == -1){
    perror("Could not unlock the status file");
  }
  

  return;

}

void ctcp_probe(clictcp_sock* csk, int pin)
{
    if (!csk->ctcp_probe) return; // logging disabled
    
    if (!csk->db) {
        csk->db = fopen("/tmp/ctcp-probe","a");  //probably shouldn't be a hard-wired filename
        if (!csk->db) perror("An error occred while opening the /tmp/ctcp-probe log file");
    }
    
    if (csk->db) {
        fprintf(csk->db,"%f\n",
                getTime());
        fflush(csk->db);
    }
    return;
}

void log_pkt(clictcp_sock* csk, int pkt, int pkt_size)
{
    if (!csk->ctcp_probe) return; // logging disabled
    
    if (!csk->pkt_log) {
        csk->pkt_log = fopen("/tmp/ctcp-pkt_log","a");  //probably shouldn't be a hard-wired filename
        if (!csk->pkt_log) perror("An error occred while opening the /tmp/ctcp-pkt_log log file");
    }
    
    if (csk->pkt_log) {
        fprintf(csk->pkt_log,"%u,%f,%u\n",
                pkt,getTime(),pkt_size);
        fflush(csk->pkt_log);
    }
    return;
}
