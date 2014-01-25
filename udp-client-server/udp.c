#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

#include <netdb.h>

#include <arpa/inet.h>

#define PORT       "8000"
#define BUF_SIZE   1024


int createsocket (char * hostname, char * service) {
   struct addrinfo   addrhints; // Details of connection we'd like
   struct addrinfo * availaddr; // Linked list of possible addresses
   struct addrinfo * addrptr;   // Used to scan availaddr list

   int    errno;
   int    skt;

   if (hostname == NULL && service == NULL) {
      fprintf (stderr, "Cannot have both hostname and service set to NULL\n");
      exit (1);
   }

   /*
    *   Define type of connection we want...
    *
    *   - if we're a server (hostname == NULL) we typically accept requests
    *     on any available interface by setting the AI_PASSIVE flag
    *
    *   - if we're a client (hostname != NULL) the AI_PASSIVE flag is ignored
    *     (so we're safe setting it anyway)
    */
   memset (&addrhints, 0, sizeof(struct addrinfo));
   addrhints.ai_family   = AF_UNSPEC;      // AF_INET for IPv4, AF_INET6 for v6
   addrhints.ai_socktype = SOCK_DGRAM;
   addrhints.ai_flags    = AI_PASSIVE;     // if no hostname then wildcard IP

   /*
    *   Get list of available addresses matching requirements...
    *
    *   - if we're a client (hostname != NULL, service == NULL) then we should
    *     get a port through which we can contact the server
    *
    *   - if we're a server (hostname == NULL, service != NULL) we should get
    *     a list of local interfaces we should be listening on
    */
   if ((errno = getaddrinfo (hostname, service, &addrhints, &availaddr)) < 0) {
      fprintf (stderr,
               "Couldn't get address info: %s\n",
               gai_strerror (errno));
      exit (1);
   }

   /*
    *   Scan list of matching addresses to find one that works
    */
   addrptr = availaddr;
   while (addrptr) {
      skt = socket (addrptr->ai_family,
                    addrptr->ai_socktype,
                    addrptr->ai_protocol);

      if (skt > 0) {
         if (hostname) {  // We're the client so connect...
            if (connect(skt, addrptr->ai_addr, addrptr->ai_addrlen) >= 0)
               break;     // We're done so exit loop with skt ready for use

         } else {         // We're the server so bind and wait...
            if (bind(skt, addrptr->ai_addr, addrptr->ai_addrlen) == 0)
               break;     // We're done so exit loop with skt ready for use
         }

         close (skt);  // Clean up ready to try next
      }

      addrptr = addrptr->ai_next;
   }

   if (addrptr == NULL) { // Reached end of list and still no connection
      fprintf (stderr,
               "Unable to establish connection on any available port\n");
      exit (1);
   }

   freeaddrinfo (availaddr);  // We're done with this list, so clean up

   return skt;
}

void
server ( void ) {
   struct sockaddr_storage peer_addr;
   socklen_t  peer_addrlen;
   ssize_t    bytesread;
   int        skt;
   int        errno;
   int        i;
   char       buff[BUF_SIZE];
   char       host[NI_MAXHOST];
   char       service[NI_MAXSERV];

   skt = createsocket ( NULL, PORT );

   for (;/*ever*/;) {
      peer_addrlen = (socklen_t) sizeof (struct sockaddr_storage);
      memset (buff, 0, BUF_SIZE);

      bytesread = recvfrom (skt, buff, BUF_SIZE, 0,
           (struct sockaddr *) &peer_addr, &peer_addrlen);

      if (bytesread <= 0) continue; // Ignore bad requests
	
	
      /*
       *   Get details of client sending message...
       */
      errno = getnameinfo ((struct sockaddr *) &peer_addr, peer_addrlen,
                           host,    NI_MAXHOST,
                           service, NI_MAXSERV, NI_NUMERICSERV);

      if (errno) fprintf (stderr, "getnameinfo(): %s\n", gai_strerror(errno));
      else {
         printf ("recvd %ld bytes from %s:%s [",
                (long) bytesread, host, service);
         for (i = 0; i < 10 && i < bytesread; ++i) putchar (buff[i]);
         printf ("]\n");
      }

      /*
       *   For the sake of something to do, we just echo the message back
       *
       *   - notice that, as we have all the peer's details from the incoming
       *     message, we can simply use sendto( )
       *
       *   - the client could have used this instead of using connect( )
       *     followed by write( ), sendto( ) essentially does both in one call
       */
      if (sendto(skt, buff, bytesread, 0,
                 (struct sockaddr *) &peer_addr, peer_addrlen) != bytesread)
         fprintf (stderr, "Couldn't echo message\n");
   }
}
/*sends udp datagrams, prints replies*/
void client (char * servername) {
   ssize_t          bytes;
   int              skt, retval;
   char             buff[BUF_SIZE];
   unsigned long    sqnum, nsqnum;   
   struct           timeval tv;
   fd_set rfds; //read file descriptor set.
	
   skt = createsocket ( servername, PORT );
	   for(sqnum=0;/*ever*/;sqnum++){
		if(sqnum==50)
			sqnum=0;
		//clear and prepare file descriptor sets
		FD_CLR(skt ,&rfds);
		FD_SET(skt, &rfds);
		//set up timer
	   	tv.tv_sec=1;
		tv.tv_usec=0;
		//(re)initialize retaval
		retval=0;
		//conv from host network sequence num.
		nsqnum = htonl(sqnum);
	   	bytes = (ssize_t) sizeof(nsqnum);
		//printf("%lu\n",sqnum);
		retval = select(skt+1, &rfds, NULL, NULL, &tv);
		//printf("retval: %d\n",retval); 	
		if(retval==-1){
			perror("select\n");
			exit(EXIT_FAILURE);
		}else if(retval==0){
	   		if(write (skt, &nsqnum, bytes) < bytes) 
	   	   		fprintf (stderr, "WARNING: write didn't accept complete message\n");
		}else{
		  if(FD_ISSET(skt, &rfds)){
   			memset (buff, 0, BUF_SIZE);
 			bytes = read (skt, buff, BUF_SIZE);
   			if (bytes < 0) {
      				perror ("read()");
      				exit (1);
   			}
		 }
  	 		printf ("Server echoed the following: %s\n", buff);
			select(0, NULL, NULL, NULL, &tv);
		}
	}
}

int main (int argc, char ** argv) {
   if (argc < 2) {
      printf ("Starting server...\n");
      server ( );

   } else {
      printf ("Contacting server %s...\n", *++argv);
      client ( *argv );
   }
   return 0;
}
