#include <iostream>
#include <vector>
#include <queue>


using namespace std;

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)


struct topic {
    char name[51];
};
struct client {
    char id[20];
    int current_socket;
    vector<topic> topics;
};

struct udp_msg {
    char topic[50];
    unsigned char type;
    char payload[1501];
};

struct message {
    udp_msg content;
    char addr_port[100];
    char sub_unsub;
};