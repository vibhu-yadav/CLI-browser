#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <time.h>
#include <poll.h>

#define BUFFER_SIZE 102400
#define SMALL_BUFFER_SIZE 12800
#define MSG_SIZE 3200

void print_protoent(struct protoent *protoent)
{
    printf("\n\nprotoent -> p_name = %s\n", protoent->p_name);
    printf("protoent -> p_proto = %d\n", protoent->p_proto);
    for (int i = 0; protoent->p_aliases[i] != NULL; i++)
        printf("protoent -> p_aliases[%d] = %s\n", i, protoent->p_aliases[i]);
}

void print_hostent(struct hostent *hostent)
{
    printf("\n\nhostent -> h_name = %s\n", hostent->h_name);
    for (int i = 0; hostent->h_aliases[i] != NULL; i++)
        printf("hostent -> h_aliases[%d] = %s\n", i, hostent->h_aliases[i]);
    printf("hostent -> h_addrtype = %d\n", hostent->h_addrtype);
    printf("hostent -> h_length = %d\n", hostent->h_length);

    for (int i = 0; hostent->h_addr_list[i] != NULL; i++)
    {

        for (int j = 0; j < hostent->h_length; j++)
            printf("%d ", hostent->h_addr_list[i][j]);
        printf("hostent -> h_addr_list[%d] = %s.\n", i, inet_ntoa(*(struct in_addr *)(hostent->h_addr_list[i])));
        printf("hostent -> h_addr_list[%d] = %s.\n", i, inet_ntoa(*(struct in_addr *)*(hostent->h_addr_list)));
    }
}

void clear(char *buffer, int size)
{
    for (int i = 0; i < size; i++)
        buffer[i] = '\0';
}

// We assume url of the form http://<hostname><path>:<port>
// This function extracts the host_addr, path and port from the url
int process_url(char *url,char* hostname, in_addr_t *host_addr, char *path, int *port)
{
    *port = 80;

    // Validate the URL
    if (strncmp(url, "http://", 7) != 0)
    {
        fprintf(stderr, "Invalid URL! URL must start with \"http://\"\n");
        return -1;
    }

    // Extract the hostname and path from the URL
    int i = 7;
    char *addr;

    // Case when Hostname is using DNS
    if ((url[i] >= 'A' && url[i] <= 'Z') || (url[i] >= 'a' && url[i] <= 'z'))
    {
        while (url[i] != '/' && url[i] != '\0')
        {
            hostname[i - 7] = url[i];
            i++;
        }
        hostname[i - 7] = '\0';
        // printf("hostname : %s\n", hostname);

        // Get the hostent struct (host entry) for the hostname
        struct hostent *hostent;
        hostent = gethostbyname(hostname);
        if (!hostent)
        {
            fprintf(stderr, "gethostbyname Failed for hostname : %s\n", hostname);
            exit(EXIT_FAILURE);
        }
        // print_hostent(hostent);

        addr = inet_ntoa(*(struct in_addr *)*(hostent->h_addr_list));
        // printf("\n\naddr : %s\n", addr);
    }
    // Case when IP of host is given
    else
    {
        // IPv4 addresses will have 3*4(numbers) + 3(dots) + 1 ('\0')
        addr = malloc(sizeof(char) * 16);
        while (url[i] != '/' && url[i] != '\0')
        {
            addr[i - 7] = url[i];
            i++;
        }
        addr[i - 1] = '\0';
        // printf("\n\naddr : %s\n", addr);
    }

    strcpy(hostname, addr);
    hostname[strlen(addr)] = '\0';

    *host_addr = inet_addr(addr);
    // printf("host_addr = %u\n", *host_addr);

    int j = 0;
    while (url[i] != '\0' && url[i] != ':')
        path[j++] = url[i++];
    path[j] = '\0';

    if (url[i] == ':')
    {
        i++;
        *port = 0;
        while (url[i] != '\0')
        {
            *port = *port * 10 + (url[i] - '0');
            i++;
        }
    }

    return 0;
}

int parse_url(char *url, char *hostname, in_addr_t *host_addr, char *path, int *port)
{

    // Default port is 80
    *port = 80;

    char protocol[10];

    // Parse the URL assumed to be of form http://<hostname><path>:<port>
    if (sscanf(url, "%[^:]://%[^/]/%[^:]:%d", protocol, hostname, path, &port) != 4)
    {
        // Parse the URL assumed to be of form http://<hostname><path>
        if (sscanf(url, "%[^:]://%[^/]/%[^\n]", protocol, hostname, path) != 3)
        {
            printf("Invalid URL\n");
            return -1;
        }
    }

    if (strcmp(protocol, "http") != 0)
    {
        printf("Invalid protocol\n");
        return -1;
    }

    // If we've been given an IP address, convert it to a host address
    if (hostname[0] >= '0' && hostname[0] <= '9')
        *host_addr = inet_addr(hostname);
    // If we've been given a hostname, convert it to a host address
    else
    {
        struct hostent *hostent;
        hostent = gethostbyname(hostname);
        if (!hostent)
        {
            fprintf(stderr, "gethostbyname Failed for hostname : %s\n", hostname);
            // exit(EXIT_FAILURE);
            return -1;
        }
        // print_hostent(hostent);

        char *addr = inet_ntoa(*(struct in_addr *)*(hostent->h_addr_list));
        // printf("\n\naddr : %s\n", addr);
        *host_addr = inet_addr(addr);
    }

    // Get rid of first '/' in path if present
}

int connect_to_server(in_addr_t in_addr, int port_no)
{

    // 255.255.255.255 is a special url that is not sent to the router
    if (in_addr == (in_addr_t)-1)
    {
        fprintf(stderr, "Internet Address Error:  inet_addr(\"%d\")\n", in_addr);
        // exit(EXIT_FAILURE);
        return -1;
    }

    int sockfd;
    /*
    struct protoent *protoent;
    protoent = getprotobyname("tcp");

    if (!protoent)
    {
        perror("getprotobyname");
        //exit(EXIT_FAILURE);
        return -1;
    }

    // Socket protocol can be manually specified using the protoent struct
    // or set as 0 in which case it will be automatically set to the default protocol for the specified socket type
        if ((sockfd = socket(AF_INET, SOCK_STREAM, protoent->p_proto)) < 0)
    {
        perror("Socket Creation Failed");
        //exit(EXIT_FAILURE);
        return -1;
    }

    */

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket Creation Failed");
        // exit(EXIT_FAILURE);
        return -1;
    }

    struct sockaddr_in serv_addr;

    serv_addr.sin_addr.s_addr = in_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_no);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server with IP : %s at Port: %d", inet_ntoa(serv_addr.sin_addr), port_no);

    return sockfd;
}

char *current_time(time_t *t)
{
    time(t);
    struct tm local_t = *localtime(t);

    char *date_time = (char *)malloc((sizeof(char)) * SMALL_BUFFER_SIZE);

    strftime(date_time, SMALL_BUFFER_SIZE, "%a, %d %b %Y %H:%M:%S %Z", &local_t);

    return date_time;
}

char *if_modified_date_time(time_t *t)
{
    int days = 5000;
    time_t new_t = *t - days * (24 * 60 * 60); // Go back $days days in time
    struct tm local_t = *localtime(&new_t);

    char *date_time = (char *)malloc((sizeof(char)) * SMALL_BUFFER_SIZE);

    strftime(date_time, SMALL_BUFFER_SIZE, "%a, %d %b %Y %H:%M:%S %Z", &local_t);

    return date_time;
}

char *get_accept_type(char *filename)
{
    // File Extension is the last part of the filename after the last '.'
    // If file extension is not null, move past the '.' to get the file extension
    char *type = (strrchr(filename, '.')) ? strrchr(filename, '.') + 1 : NULL;

    char *accept_type = (char *)malloc((sizeof(char)) * SMALL_BUFFER_SIZE);

    // If no file extension is present, return text/*
    if (!type)
        return strdup("text/*");

    if (!strcmp(type, "html"))
        return strdup("text/html");

    if (!strcmp(type, "pdf"))
        return strdup("application/pdf");

    if (!strcmp(type, "jpg") || !strcmp(type, "jpeg"))
        return strdup("image/jpeg");

    if (!strcmp(type, "png"))
        return strdup("image/png");

    // This is the default accept type, interpret as plain-text
    return strdup("text/*");
}

char *get_status_message(int status_code)
{
    switch (status_code)
    {

    case 200:
        return strdup("OK");

    case 400:
        return strdup("Bad Request");

    case 403:
        return strdup("Forbidden");

    case 404:
        return strdup("Not Found");

    default:
        return strdup("Unknown Issue");
    }

}

void send_request(int sockfd, char *req, FILE *fp)
{

    int n;
    char buffer[MSG_SIZE + 1];
    clear(buffer, MSG_SIZE);

    // printf("[%d]\n%s\n", strlen(req), req);
    if ((n = send(sockfd, req, strlen(req), 0)) < 0)
    {
        perror("Sending Request Failed");
        return;
    }

    if (!fp)
        return;

    while ((n = fread(buffer, sizeof(char), MSG_SIZE - 1, fp)) > 0)
    {
        // Send the bytes that were read
        send(sockfd, buffer, n, 0);
        buffer[n] = '\0';
        // printf("[%d]\n%s",n,buffer);

        clear(buffer, MSG_SIZE + 1);
    }

    printf("\nFile Sent Successfully\n");
}

void recv_put_response(int sockfd)
{
    // Recieve Header
    char header[BUFFER_SIZE + 1];
    clear(header, BUFFER_SIZE + 1);

    char buffer[MSG_SIZE + 1];
    clear(buffer, MSG_SIZE + 1);

    int n;

    while( (n = recv(sockfd,buffer,MSG_SIZE,0)) > 0 ) {
        strcat(header,buffer);
        clear(buffer, MSG_SIZE + 1);

        if(strstr(header,"\r\n\r\n") != NULL)
            break;
    }

    printf("Response recived as : \n%s\n", header);

    char* temp = (strstr(header, " ")) + 1;

    int status_code = (temp[0]-'0') * 100 + (temp[1]-'0') * 10 + (temp[2]-'0');
    char* status_message = get_status_message(status_code);

    printf("Server Response :\n");
    printf("%s [%d]\n", status_message, status_code);

}

void recv_get_response(int sockfd,char* filename)
{
    char header[BUFFER_SIZE + 1];
    clear(header, BUFFER_SIZE + 1);

    char buffer[MSG_SIZE + 1];
    clear(buffer, MSG_SIZE + 1);

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    int timeout = 3; // Timeout in seconds

    int p = poll(fds, 1, timeout * 1000);

    if(p < 0){
        perror("Polling Error");
        return;
    }

    if(p == 0){
        printf("Timeout, No response from server.\n");
        return;
    }

    int n, len = 0;
    char* body_start;

    // Recieve Header
    while( (n = recv(sockfd,buffer,MSG_SIZE,0)) > 0 ){
        len += n;
        buffer[n] = '\0';
        strcat(header,buffer);

        // Header ends with a "\r\n\r\n"
        // But this bit can itself arrive in chunks, so we concatenate everything to header
        // and break off when we find the "\r\n\r\n"
        body_start = strstr(header,"\r\n\r\n");

        if(body_start){
            body_start += 4;
            break;
        }
    }



    // Move the part of header that contains the body to the body
    char body[BUFFER_SIZE + 1];
    clear(body, BUFFER_SIZE + 1);
    strcpy(body, body_start);

    // Clear the part of header that contains the body
    clear(body_start,strlen(body_start)+1);

    printf("Response recived as : \n%s\n", header);
    printf("%d %d\n", strlen(header), strlen(body));
    // Find status code from header
    char* temp = (strstr(header, " ")) + 1;
    int status_code = (temp[0]-'0') * 100 + (temp[1]-'0') * 10 + (temp[2]-'0');
    char* status_message = get_status_message(status_code);

    if(status_code != 200){
        printf("Server Response :\n");
        printf("%s [%d]\n", status_message, status_code);
        return;
    }

    printf("\nRecieving File...\n");

    temp = strstr(header, "Content-Length: ");
    int content_length;
    sscanf(temp, "Content-Length: %d", &content_length);

    // Check if the file already exists
    // FILE* fp = fopen(filename,"r");

    FILE* fp;
    if( (fp = fopen(filename,"wb") ) == NULL ){
        perror("File opening failed");
        return;
    }

    len = strlen(body);
    fwrite(body, sizeof(char), strlen(body), fp);
    clear(body, BUFFER_SIZE + 1);

    while(len < content_length){


        n = recv(sockfd,buffer,MSG_SIZE,0);
        fwrite(buffer, sizeof(char), n, fp);

        if(n <= 0)
            break;
        len += n;
        printf("Recieved %d bytes (%d,%d) \n",n,len,content_length);

        // strcat(body,buffer);
        // printf("%s",buffer);
        clear(buffer, MSG_SIZE + 1);
    }

    fclose(fp);

    // To open the file in the default application
    temp = strstr(header, "Content-Type: ");
    char content_type[BUFFER_SIZE + 1];
    sscanf(temp, "Content-Type: %s", content_type);

    // int pid = fork();
    // if(pid == 0){

    //     if(execlp("xdg-open","xdg-open",filename,NULL) == -1){
    //         perror("xdg-open failed");
    //         return;
    //     }

    // }

}

void GET(char *url)
{
    char path[BUFFER_SIZE], hostname[BUFFER_SIZE];
    int port;
    in_addr_t host_addr;


    // int ret = process_url(url, &host_addr, path, &port);
    // int ret = parse_url(url, hostname, &host_addr, path, &port);
    int ret = process_url(url,hostname, &host_addr, path, &port);

    // printf("Hostname : %s\n", hostname);

    if (ret == -1)
    {
        printf("Invalid URL, Please try again with a valid URL.\n");
        return;
    }


    char *filename = (strrchr(path, '/')) ? strrchr(path, '/') + 1 : path;

    int sockfd = connect_to_server(host_addr, port);

    // Send the GET request
    char request[BUFFER_SIZE];

    time_t t;
    char *date_time = current_time(&t);
    char *if_modified_since = if_modified_date_time(&t);
    char *accept_type = get_accept_type(filename);
    char *request_template = "GET %s HTTP/1.1\r\n"
                             "Host: %s\r\n"
                             "Connection: close\r\n"
                             "Date: %s\r\n"
                             "Accept: %s\r\n"
                             "Accept-Language: en-US\r\n"
                             "If-Modified-Since: %s\r\n"
                             "\r\n";

    snprintf(request, BUFFER_SIZE, request_template, path
    // , host_addr
    , hostname
    , date_time
    , accept_type, if_modified_since
    );

    // printf("\nRequest to be sent...\n%s\n", request);

    printf("\n REQUEST SENT \n%s",request);

    send_request(sockfd, request, NULL);

    recv_get_response(sockfd, filename);

}

void PUT(char *url, char *filepath)
{
    char hostname[BUFFER_SIZE], path[BUFFER_SIZE];
    int port;
    in_addr_t host_addr;

    // int ret = parse_url(url, hostname, &host_addr, path, &port);
    int ret = process_url(url, hostname, &host_addr, path, &port);
    if (ret == -1)
    {
        printf("Invalid URL, Please try again with a valid URL.\n");
        return;
    }

    char *filename = (strrchr(filepath, '/')) ? strrchr(filepath, '/') + 1 : filepath;

    int sockfd = connect_to_server(host_addr, port);

    // Send the PUT request
    char request[BUFFER_SIZE];

    time_t t;
    char *date_time = current_time(&t);
    char *if_modified_since = if_modified_date_time(&t);
    char *content_type = get_accept_type(filename);

    FILE *fp;
    if ((fp = fopen(filepath, "r")) == NULL)
    {
        printf("File [%s] not found\n",filepath);
        return;
    }
    fseek(fp, 0L, SEEK_END);
    int content_length = ftell(fp);
    fclose(fp);

    char *request_template = "PUT %s/%s HTTP/1.1\r\n"
                             "Host: %s\r\n"
                             "Date: %s\r\n"
                             "Content-Length: %d\r\n"
                             "Content-Type: %s\r\n"
                             "Content-Language: en-US\r\n"
                             "Connection: close\r\n"
                             "\r\n";

    snprintf(request, BUFFER_SIZE, request_template, path, filename, hostname, date_time, content_length, content_type);
    printf("Request to be sent...\n%s\n", request);

    fp = fopen(filepath, "r");
    send_request(sockfd, request, fp);
    fclose(fp);

    recv_put_response(sockfd);
}

int main()
{

    while (1)
    {
        printf("MyBroswer> ");

        char request_type[128];
        scanf("%s", request_type);

        if (strcmp(request_type, "QUIT") == 0)
        {
            printf("Exiting...\n");
            break;
        }

        // GET request supporting hostname being either IP address or domain name
        else if (strcmp(request_type, "GET") == 0)
        {
            // As Microsoft Edge and Internet Explored have a maximum URL length of 2048 characters,
            // We feel it is safe to assume that the URL length will not exceed 2048 characters for a simple browser like this
            char url[2048];
            scanf("%s", url); // We assume the URL will be entered without any spaces (spaces replaced by %20)

            // printf("Attempting a GET request for URL : %s\n", url);

            GET(url);
        }

        else if (strcmp(request_type, "PUT") == 0)
        {
            char url[2048], file_path[2048];
            scanf("%s ", url); // We assume the URL will be entered without any spaces (spaces replaced by %20)
            fgets(file_path, 2048, stdin);
            file_path[strlen(file_path) - 1] = '\0';

            // printf("Attempting a PUT request for URL : %s, file_path : %s\n", url, file_path);

            PUT(url, file_path);
        }

        else
            printf("Invalid Request Type. Please Try Again.\n");
    }

    return 0;
}