#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>


/*
	[DNS MSG]
	https://datatracker.ietf.org/doc/html/rfc1035#section-4.1
		+---------------------+
		|        Header       |
		+---------------------+
		|       Question      | the question for the name server
		+---------------------+
		|        Answer       | RRs answering the question
		+---------------------+
		|      Authority      | RRs pointing toward an authority
		+---------------------+
		|      Additional     | RRs holding additional information
		+---------------------+
*/


#pragma pack(push, 1)

typedef struct DNS_HEADER {
	uint16_t id;       // Packet Identifier
	uint16_t flags;		// Flags (16 bits total)
    uint16_t qdcount;	// Question Count
    uint16_t ancount;	// Answer Record Count
    uint16_t nscount;	// Authority Record Count
    uint16_t arcount;	// Additional Record Count
} DNS_HEADER_t;

#pragma pack(pop)

/*
// flags
// | QR | OPCODE | AA | TC | RD | RA | Z | RCODE |
// | 1  |   4    | 1  | 1  | 1  | 1  | 3 |   4   |
*/
#define SET_DNS_QR(flags, val)    ((flags) = ((flags) & 0b0111111111111111) | ((val) << 15))
#define SET_DNS_OPCODE(flags, val)((flags) = ((flags) & 0b1000011111111111) | ((val) << 11))
#define SET_DNS_AA(flags, val)    ((flags) = ((flags) & 0b1111011111111111) | ((val) << 10))
#define SET_DNS_TC(flags, val)    ((flags) = ((flags) & 0b1111101111111111) | ((val) << 9))
#define SET_DNS_RD(flags, val)    ((flags) = ((flags) & 0b1111110111111111) | ((val) << 8))
#define SET_DNS_RA(flags, val)    ((flags) = ((flags) & 0b1111111011111111) | ((val) << 7))
#define SET_DNS_Z(flags, val)     ((flags) = ((flags) & 0b1111111110001111) | ((val) << 3))
#define SET_DNS_RCODE(flags, val) ((flags) = ((flags) & 0b1111111111110000) | (val))


int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	printf("size of DNS HEADER : %ld \n", sizeof(DNS_HEADER_t));
	static_assert(sizeof(DNS_HEADER_t) == 12, "DNS HEADER size should 12, packed \n");

	
	// datagram = UDP
	int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocket == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting REUSE_PORT
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { 
		.sin_family = AF_INET ,
		.sin_port = htons(2053),
		.sin_addr = { htonl(INADDR_ANY) },
	};
	
	if (bind(udpSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

   	int bytesRead; char buffer[512];
   	struct sockaddr_in clientAddress;
   	socklen_t clientAddrLen = sizeof(clientAddress);


   
   while (1) {
       // 데이터 넣어준 주소를 clientAddress에 넣어준다.
       bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddress, &clientAddrLen);
       if (bytesRead == -1) {
           perror("Error receiving data");
           break;
       }
   
       buffer[bytesRead] = '\0';
       printf("Received %d \n", bytesRead);
   
       // return response
	   DNS_HEADER_t stResponse;
	   memset(&stResponse, 0, sizeof(stResponse));
	   stResponse.id = htons(1234);
	   SET_DNS_QR(stResponse.flags, 1);
	   stResponse.flags = htons(stResponse.flags);
   
       // Send response
       if (sendto(udpSocket, (void*)&stResponse, sizeof(DNS_HEADER_t), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) == -1) {
           perror("Failed to send response");
       }
   }
   
   close(udpSocket);

   return 0;
}

