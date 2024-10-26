#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

void send_message(int fd, const char *buf);
void send_looped(int fd, const void *buf, size_t size);
void receive_loop(int fd, void *buf, size_t size);
char *receive_message(int fd);

int main(int argc, char *argv[])
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket()");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3000);
    const char* ipaddr = "127.0.0.1";
    
    if (inet_pton(AF_INET, ipaddr, &addr.sin_addr) != 1)
    {
        fprintf(stderr, "inet_pton(%s)\n", ipaddr);
        exit(1);
    }

    if (connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        printf("Unable to connect to elevator system.\n");
        exit(1);
    }

    if(strcmp(argv[1], argv[2]) == 0)
    {
        printf("You are already on that floor!\n");
    }
    else
    {
        char buf[1024];

        strcpy(buf, "CALL ");
        strcat(buf, argv[1]);
        strcat(buf, " ");
        strcat(buf, argv[2]);

        send_message(sockfd, buf);


        socklen_t addr_len = sizeof(addr);
        accept(sockfd, (struct sockaddr *)&addr, &addr_len);
        char *message = receive_message(sockfd);

        int i = 0;
        char *msg_array[10];
        char *split_token = strtok(message, " ");

        while (split_token != NULL)
        {
            msg_array[i] = split_token;
            split_token = strtok(NULL, " ");
            i++;
        }

        char response[100];
        if (strcmp(msg_array[0], "CAR") == 0)
        {
            strcpy(response, "Car ");
            strcat(response, msg_array[1]);
            strcat(response, " is arriving.");
        }
        else if (strcmp(msg_array[0], "UNAVAILABLE") == 0)
        {
            printf("Sorry, no car is available to take this request.");
        }

        printf("%s\n", response);

        free(message);
    }



    if (shutdown(sockfd, SHUT_RDWR) == -1)
    {
        perror("shutdown()");
        exit(1);
    }

    if (close(sockfd) == -1)
    {
        perror("close()");
        exit(1);
    }
}

void send_message(int fd, const char *buf)
{
    uint32_t len = htonl(strlen(buf));
    send_looped(fd, &len, sizeof(len));
    send_looped(fd, buf, strlen(buf));
}

void send_looped(int fd, const void *buf, size_t size)
{
    const char *ptr = buf;
    size_t remain = size;

    while (remain > 0)
    {
        ssize_t sent = write(fd, ptr, remain);
        if (sent == -1)
        {
            perror("write()");
            exit(1);
        }
        ptr += sent;
        remain -= sent;
    }
}

void receive_loop(int fd, void *buf, size_t size)
{
    char *ptr = buf;
    size_t remain = size;

    while (remain > 0)
    {
        ssize_t received = read(fd, ptr, remain);
        if (received == -1)
        {
            perror("read()");
            exit(1);
        }
        ptr += received;
        remain -= received;
    }
}

char *receive_message(int fd)
{
    uint32_t nlen;
    receive_loop(fd, &nlen, sizeof(nlen));
    uint32_t len = ntohl(nlen);

    char *buf = malloc(len + 1);
    buf[len] = '\0';
    receive_loop(fd, buf, len);
    return buf;
}