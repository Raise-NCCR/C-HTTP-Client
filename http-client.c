#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <unistd.h>

#define BUF_LEN 256

void http_request(char *URL, int *s, char *host, char *path, unsigned short *port)
{
    struct hostent *servhost;
    struct sockaddr_in server;
    struct servent *service;

    char host_path[BUF_LEN];

    if (strlen(URL) > BUF_LEN - 1)
    {
        fprintf(stderr, "URL が長すぎます。\n");
        exit(EXIT_FAILURE);
    }

    if (strstr(URL, "http://") &&
        sscanf(URL, "http://%s", host_path) &&
        strcmp(URL, "http://"))
    {
        char *p;

        p = strchr(host_path, '/');
        if (p != NULL)
        {
            strcpy(path, p);
            *p = '\0';
            strcpy(host, host_path);
        }
        else
        {
            strcpy(host, host_path);
        }

        p = strchr(host, ':');
        if (p != NULL)
        {
            *port = atoi(p + 1);
            if (*port <= 0)
            {
                *port = 80;
            }
            *p = '\0';
        }
    }
    else
    {
        fprintf(stderr, "URL は http://host/path の形式で指定してください。\n");
        exit(EXIT_FAILURE);
    }

    printf("http://%s%s を取得します。\n\n", host, path);

    servhost = gethostbyname(host);
    if (servhost == NULL)
    {
        fprintf(stderr, "[%s] から IP アドレスへの変換に失敗しました。\n", host);
        exit(EXIT_FAILURE);
    }

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;

    bcopy(servhost->h_addr, &server.sin_addr, servhost->h_length);

    if (*port != 0)
    {
        server.sin_port = htons(*port);
    }
    else
    {
        service = getservbyname("http", "tcp");
        if (service != NULL)
        {
            server.sin_port = service->s_port;
        }
        else
        {
            server.sin_port = htons(80);
        }
    }

    if ((*s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "ソケットの生成に失敗しました。\n");
        exit(EXIT_FAILURE);
    }

    if (connect(*s, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        fprintf(stderr, "connect に失敗しました。\n");
        exit(EXIT_FAILURE);
    }
}

char *http_get(char *URL)
{
    int s;
    char host[BUF_LEN] = "";
    char path[BUF_LEN] = "/";
    unsigned short port = 80;

    char send_buf[BUF_LEN];

    http_request(URL, &s, &(host[0]), &(path[0]), &port);

    sprintf(send_buf, "GET %s HTTP/1.0\r\n", path);
    write(s, send_buf, strlen(send_buf));

    sprintf(send_buf, "Host: %s:%d\r\n", host, port);
    write(s, send_buf, strlen(send_buf));

    sprintf(send_buf, "\r\n");
    write(s, send_buf, strlen(send_buf));

    char *res = (char *)calloc(1, 100000);
    while (1)
    {
        char buf[BUF_LEN];
        int read_size;
        read_size = read(s, buf, BUF_LEN);
        if (read_size > 0)
        {
            strncat(res, buf, read_size);
        }
        else
        {
            break;
        }
    }

    close(s);

    return res;
}
