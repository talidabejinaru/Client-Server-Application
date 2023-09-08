#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cmath>

#include <iostream>

#include "helpers.h"

using namespace std;


void usage(char *file) {
    fprintf(stderr, "Usage: %s server_address server_port\n", file);
    exit(0);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    struct sockaddr_in serv_addr;
    int sockfd, n, ret;
    struct message msg;

    if (argc < 3) {
        usage(argv[0]);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd == -1, "sockfd");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    inet_aton(argv[2], &serv_addr.sin_addr);

    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    if(strlen(argv[1]) < 10)
        ret = send(sockfd, argv[1], strlen(argv[1]), 0);
    else
        return 0;
    DIE(ret < 0, "send");

    fd_set read_fds;
    fd_set tmp_fds;

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    FD_SET(0, &read_fds);
    FD_SET(sockfd, &read_fds);

    while (1) {
        tmp_fds = read_fds;
        ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            char input[1000], *token;
            memset(input, 0, 1000);
            fgets(input,  999, stdin);

            token = strtok(input, " \n");
            if(!strcmp(token, "exit")) {
                n = send(sockfd, NULL, 0, 0);
                DIE(n < 0, "send");
                return 0;
            } else if(!strncmp(token, "subscribe", 9)) {
                cout << "Subscribed to topic." << endl;
                msg.sub_unsub = 's';
            } else if(!strncmp(token, "unsubscribe", 11)) {
                cout << "Unsubscribed from topic." << endl;
                msg.sub_unsub = 'u';   
            }

            token = strtok(NULL, " \n");
            if(token == NULL)
                continue;
            memset(msg.content.topic, 0, 50);
            memset(&msg.content.type, 0, 1);
            memset(msg.content.payload, 0, 1501);
            memset(msg.addr_port, 0, 100);
            strcpy(msg.content.topic, token);

            n = send(sockfd, &msg, sizeof(struct message), 0);
            DIE(n < 0, "send");

        } else if (FD_ISSET(sockfd, &tmp_fds)) {
            n = recv(sockfd, &msg, sizeof(struct message), 0);
            DIE(n < 0, "recv");
            char output[1501];

            if(n == 0) {
                return 0;
            } else {
                if(msg.content.type == 0) {

                    strcat(msg.addr_port, "INT - ");
                    uint32_t num = ntohl(*(uint32_t*)(msg.content.payload + 1));
                    if((unsigned char)msg.content.payload[0] == 1)
                        num *= -1;
                    sprintf(output, "%d", num);

                } else if(msg.content.type == 1) {

                    strcat(msg.addr_port, "SHORT_REAL - ");
                    sprintf(output, "%.2f", (float)ntohs(*(uint16_t*)(msg.content.payload))/100);

                } else if(msg.content.type == 2) {

                    strcat(msg.addr_port, "FLOAT - ");
                    float num = ntohl(*(uint32_t*)(msg.content.payload + 1))/pow(10, (unsigned char)msg.content.payload[5]);
                    if ((unsigned char)msg.content.payload[0] == 1)
                        num *= -1;
                    sprintf(output, "%lf", num);

                } else if(msg.content.type == 3) {

                    strcat(msg.addr_port, "STRING - ");
                    strcpy(output, msg.content.payload);

                }

                cout << msg.addr_port << output << endl;
            }
        }
    }

    close(sockfd);

    return 0;
}
