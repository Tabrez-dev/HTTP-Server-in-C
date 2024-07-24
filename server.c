#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#define BUFFER_SIZE 1024

int main() {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    printf("Logs from your program will appear here!\n");

    // Uncomment this block to pass the first stage
    int server_fd, client_addr_len;
    struct sockaddr_in client_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("SO_REUSEADDR failed: %s \n", strerror(errno));
        return 1;
    }

    struct sockaddr_in serv_addr = { .sin_family = AF_INET,
                                     .sin_port = htons(4221),
                                     .sin_addr = { htonl(INADDR_ANY) },
                                   };

    if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    printf("Waiting for a client to connect...\n");
    client_addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
    printf("Client connected\n");

    if (client_fd < 0) {
        perror("accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv failed");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

//    buffer[bytes_received] = '\0'; // Null-terminate the received data

    // Extract the request path from the request line
    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");
    strtok(NULL, "\r\n"); // Skip the HTTP version part

    // Prepare the HTTP response
    const char *response;

    if (path != NULL) {
        if (strcmp(path, "/") == 0) {
            // Respond with 200 OK for path "/"
            response = "HTTP/1.1 200 OK\r\n\r\n";
        } else if (strncmp(path, "/echo/", 6) == 0) {
            char *str = path + 6; // Extract the string part

            // Create the response
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: %zu\r\n\r\n" // CRLF to end headers
                     "%s",
                     strlen(str), str);

            // Send the response to the client
            ssize_t bytes_sent = send(client_fd, response, strlen(response), 0);
            if (bytes_sent < 0) {
                perror("send failed");
            } else {
                printf("HTTP response sent\n");
            }
        } else if(strcmp(path,"/user-agent")==0){
		char *user_agent=NULL;
		char *line=strtok(NULL,"\r\n");
		while(line!=NULL){
			if(strncasecmp(line,"User-Agent:",11)==0){
				user_agent=line+12;
				break;
			}
			line=strtok(NULL,"\r\n");
		}
		  if (user_agent != NULL) {
                // Create the response
                char response[BUFFER_SIZE];
                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: %zu\r\n\r\n" // CRLF to end headers
                         "%s",
                         strlen(user_agent), user_agent);

                // Send the response to the client
                ssize_t bytes_sent = send(client_fd, response, strlen(response), 0);
                if (bytes_sent < 0) {
                    perror("send failed");
                } else {
                    printf("HTTP response sent\n");
                }
            } else {
                // Respond with 400 Bad Request if User-Agent is not found
                response = "HTTP/1.1 400 Bad Request\r\n\r\n";
                ssize_t bytes_sent = send(client_fd, response, strlen(response), 0);
                if (bytes_sent < 0) {
                    perror("send failed");
                } else {
                    printf("HTTP response sent\n");
                }
            }		
		
	}else {
            // Respond with 404 Not Found for any other path
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
    } else {
        // Respond with 400 Bad Request if the path is null
        response = "HTTP/1.1 400 Bad Request\r\n\r\n";
    }

    ssize_t bytes_sent = send(client_fd, response, strlen(response), 0);
    if (bytes_sent < 0) {
        perror("send failed");
    } else {
        printf("HTTP response sent\n");
    }

    // Close the sockets
    close(client_fd);
    close(server_fd);

    return 0;
}
