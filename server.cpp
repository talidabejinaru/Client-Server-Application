#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

#include "helpers.h"

using namespace std;


void usage(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    struct client *cli;
    int socktcp, sockudp, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    int n, i, ret, s = 1;
    socklen_t clilen;
    vector<client> clients;
    struct message msg;
    struct udp_msg udp;

    fd_set read_fds;
    fd_set tmp_fds;
    int fdmax;

    if (argc < 2) {
        usage(argv[0]);
    }

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    socktcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(socktcp < 0, "socktcp");
    sockudp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockudp < 0, "sockudp");


    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(socktcp, (struct sockaddr *)&serv_addr,
               sizeof(struct sockaddr));
    DIE(ret < 0, "bindtcp");

    ret = bind(sockudp, (struct sockaddr *)&serv_addr,
             sizeof(struct sockaddr_in));
    DIE(ret < 0, "bindudp");


    ret = listen(socktcp, 20);
    DIE(ret < 0, "listen");

    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(socktcp, &read_fds);
    FD_SET(sockudp, &read_fds);
    fdmax = sockudp;
    
    setsockopt(socktcp, IPPROTO_TCP, TCP_NODELAY, (void *)&s, sizeof(s));

    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) {
                    char input[1000];
                    memset(input, 0, 1000);
                    fgets(input,  999, stdin);

                    if(!strcmp(input, "exit\n")) {
                        for(int j = 0; j < clients.size(); j++)
                            if(clients[j].current_socket != -1) {
                                n = send(clients[j].current_socket, NULL, 0, 0);
                                DIE(n < 0, "send");
                                close(clients[j].current_socket);
                            }
                        close(socktcp);
                        close(sockudp);
                        return 0;
                    }
                            
                } else if (i == socktcp) {
                    clilen = sizeof(cli_addr);
                    newsockfd = accept(socktcp, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(newsockfd < 0, "accept");
                    setsockopt(socktcp, IPPROTO_TCP, TCP_NODELAY, (void *)&s, sizeof(s));

                    int flag = 0;
                    char buffer[20];
                    memset(buffer, 0, 20);
                    n = recv(newsockfd, buffer, sizeof(buffer), 0);
                    DIE(n < 0, "recv");

                    for(int j = 0; j < clients.size(); j++) {
                        if(!strcmp(clients[j].id, buffer)) {
                            if(clients[j].current_socket == -1) {
                                 cout << "New client" << " " << buffer << " connected from " 
                                 << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << "." << endl;
                                clients[j].current_socket = newsockfd;
                                
                                FD_SET(newsockfd, &read_fds);
                                if (newsockfd > fdmax)
                                fdmax = newsockfd;
                            } else {
                                cout << "Client " << buffer << " already connected." << endl;
                                n = send(newsockfd, NULL, 0, 0);
                                DIE(n < 0, "send");
                                close(newsockfd);
                            }
                            flag = 1;
                            break;
                        }
                    }

                    if(flag == 0) {
                        cout << "New client" << " " << buffer << " connected from " 
                        << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << "." << endl;
                        struct client new_cli;
                        strcpy(new_cli.id, buffer);
                        new_cli.current_socket = newsockfd;
                        clients.push_back(new_cli);

                        FD_SET(newsockfd, &read_fds);
                        if (newsockfd > fdmax)
                            fdmax = newsockfd;
                    }
                    
                } else if (i == sockudp) {
                    memset(udp.topic, 0, 50);
                    memset(&udp.type, 0, 1);
                    memset(udp.payload, 0, 1501);

                    n = recvfrom(sockudp, &udp, sizeof(struct udp_msg), 0, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(n < 0, "recvfrom");
                    udp.payload[1500] = '\0';

                    memset(msg.content.topic, 0, 50);
                    memset(&msg.content.type, 0, 1);
                    memset(msg.content.payload, 0, 1501);
                    memset(&msg.sub_unsub, 0, 1);
                    memset(msg.addr_port, 0, 100);

                    msg.content = udp;
                    strcat(msg.addr_port, inet_ntoa(cli_addr.sin_addr));
                    strcat(msg.addr_port, ":");
                    strcat(msg.addr_port, to_string(ntohs(cli_addr.sin_port)).c_str());
                    strcat(msg.addr_port, " - ");
                    strncat(msg.addr_port, msg.content.topic, 50);
                    strcat(msg.addr_port, " - ");

                    for(int j = 0; j < clients.size(); j++) {
                        if(clients[j].current_socket != -1) {
                            for(int k = 0; k < clients[j].topics.size(); k++) {
                                if(!strcmp(clients[j].topics[k].name, udp.topic)) {
                                    n = send(clients[j].current_socket, &msg, sizeof(msg), 0);
                                    DIE(n < 0, "send");
                                    break;
                                }
                            }
                        }
                    }

                } else {
                    memset(msg.content.topic, 0, 50);
                    memset(&msg.content.type, 0, 1);
                    memset(msg.content.payload, 0, 1501);
                    memset(&msg.sub_unsub, 0, 1);
                    memset(msg.addr_port, 0, 100);

                    n = recv(i, &msg, sizeof(struct message), 0);
                    DIE(n < 0, "recv");

                    int flag = 0;
                    if(n == 0) {
                        for(int j = 0; j < clients.size(); j++)
                            if(clients[j].current_socket == i) {
                                clients[j].current_socket = -1;
                                cout << "Client " << clients[j].id << " disconnected." << endl;
                                close(i);
                                FD_CLR(i, &read_fds);
                                break;
                            }
                    } else if(msg.sub_unsub == 's') {
                        for(int j = 0; j < clients.size(); j++)
                            if(clients[j].current_socket == i) {
                                for(int k = 0; k < clients[j].topics.size(); k++)
                                    if(!strcmp(clients[j].topics[k].name, msg.content.topic)){
                                        flag = 1;
                                        break;
                                    }
                            
                                if(flag == 0) {
                                    topic tpc;
                                    strcpy(tpc.name, msg.content.topic);
                                    clients[j].topics.push_back(tpc);
                                }
                                break;
                            }
                                              
                    } else if(msg.sub_unsub == 'u') {
                        for(int j = 0; j < clients.size(); j++)
                            if(clients[j].current_socket == i) {
                                for(int k = 0; k < clients[j].topics.size(); k++)
                                    if(!strcmp(clients[j].topics[k].name, msg.content.topic)){
                                        clients[j].topics.erase(clients[j].topics.begin()+k);
                                        break;
                                    }
                            
                                break;
                            }
                    }
                }
            }
        }
    }
    

    close(socktcp);
    close(sockudp);

    return 0;
}
