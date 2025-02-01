#include <netdb.h>
#include <stdio.h>        
#include <stdlib.h>       
#include <string.h>       
#include <sys/socket.h>   
#include <unistd.h>       
#include <time.h>

#include <iostream>
using namespace std;
#define CHUNKSIZE 10000

void print_usage() {
    cout << "Usage:" << endl;
    cout << "Server mode: ./iPerfer -s -p <listen_port>" << endl;
    cout << "Client mode: ./iPerfer -c -h <server_hostname> -p <server_port> -t <time>" << endl;
}

bool validate_port(int port) {
    return (port >= 1024 && port <= 65535);
}

int run_server(int port) {
    char buffer[CHUNKSIZE] = { 0 };

    // Create socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in addr; 
    memset(&addr, 0, sizeof(addr)); 
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port = htons(port); 

    // Bind the socket
    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(socket_fd);
        return 1;
    }

    // Start listening
    if (listen(socket_fd, 10) < 0) {
        perror("Listen failed");
        close(socket_fd);
        return 1;
    }

    socklen_t addr_len = sizeof(addr); 

    int one_time = 0;
    while (one_time==0) {
        one_time=1;
        struct timespec start_time, current_time;
        bool started = false;

        int conn = accept(socket_fd, (struct sockaddr *) &addr, &addr_len);
        if (conn < 0) {
            perror("Accept failed");
            close(socket_fd);
            return 1;
        }

        ssize_t byte_recved = 1;
        unsigned long long total_byte_recved = 0;
        while (byte_recved > 0) {
            byte_recved = recv(conn, buffer, sizeof(buffer), MSG_NOSIGNAL);

            if (!started) {
                started = true;
                clock_gettime(CLOCK_MONOTONIC, &start_time); // Capture start time
            }

            if (byte_recved < 0) {
                break;  // Exit the loop on error
            }

            if (byte_recved > 0) {
                buffer[byte_recved] = '\0';
                if (buffer[0] == 'a') break;
                total_byte_recved += byte_recved;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &current_time); // Capture end time

        // Calculate elapsed time in seconds (with nanosecond precision)
        double elapsed_time = (current_time.tv_sec - start_time.tv_sec) +
                              (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

        double rate = total_byte_recved * 8 / (elapsed_time * (1000 * 1000)); // Mbps
        int total_KB_recved = total_byte_recved / 1000;

        char end_msg[100];
        snprintf(end_msg, sizeof(end_msg), "Received=%d KB, Rate=%.2f Mbps", total_KB_recved, rate);

        // Send the final message to the client
        send(conn, end_msg, sizeof(end_msg), MSG_NOSIGNAL);

        // Print the final Received message
        printf("%s\n", end_msg);

        // Close the connection
        close(conn);
    }
    close(socket_fd);
    return 0;
}

int run_client(const char *hostname, int server_port, int input_time) {
    char buffer[CHUNKSIZE] = { 0 };

    // Create a socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Resolve the hostname
    struct hostent *server = gethostbyname(hostname);

    // Set up the address structure
    struct sockaddr_in addr; 
    memset(&addr, 0, sizeof(addr)); 
    addr.sin_family = AF_INET; 
    memcpy(&addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    addr.sin_port = htons(server_port);

    // Connect to the server
    if (connect(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        close(socket_fd);
        return 1;
    }

    // Prepare a message to send
    void *chunk = malloc(CHUNKSIZE);
    memset(chunk, 0, CHUNKSIZE); 

    struct timespec start_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time); // Capture start time

    ssize_t bytes_sent;
    double elapsed_time;
    do {
        clock_gettime(CLOCK_MONOTONIC, &current_time); // Capture current time

        // Calculate elapsed time in seconds (with nanosecond precision)
        elapsed_time = (current_time.tv_sec - start_time.tv_sec) +
                              (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

        bytes_sent = send(socket_fd, chunk, CHUNKSIZE, MSG_NOSIGNAL);
        if (bytes_sent < 0) {
            perror("Send failed");
        }
    } while (elapsed_time <= input_time);

    char *end_msg = (char*)malloc(CHUNKSIZE);
    memset((void*)end_msg, 'a', CHUNKSIZE); 

    send(socket_fd, end_msg, 1, MSG_NOSIGNAL);

    int byte_recved = recv(socket_fd, buffer, sizeof(buffer), MSG_NOSIGNAL);
    buffer[byte_recved] = '\0';

    if (buffer[0] == 'R') {
        clock_gettime(CLOCK_MONOTONIC, &current_time); // Capture current time

        // Calculate elapsed time in seconds (with nanosecond precision)
        elapsed_time = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
        close(socket_fd);

        int received_KB;
        sscanf(buffer, "Received=%d KB", &received_KB);
        char end_msg[100];
        double rate = received_KB * 8  / (elapsed_time * (1000 )); // Mbps

        snprintf(end_msg, sizeof(end_msg), "Sent=%d KB, Rate=%.2f Mbps", received_KB, rate);
        printf("%s\n", end_msg);
    }

    return 0;
}

int main(int argc, const char **argv) {
    
    if (argc < 2) {
        print_usage();
        return 1;
    }

    // Server mode
    if (strcmp(argv[1], "-s") == 0) {
        if (argc != 4 || strcmp(argv[2], "-p") != 0) {
            cout << "Error: missing or extra arguments" << endl;
            return 1;
        }

        int port = atoi(argv[3]);
        if (!validate_port(port)) {
            cout << "Error: port number must be in the range of [1024, 65535]" << endl;
            return 1;
        }

        run_server(port);
    }
    // Client mode
    else if (strcmp(argv[1], "-c") == 0) {
        if (argc != 8 || strcmp(argv[2], "-h") != 0 || strcmp(argv[4], "-p") != 0 || strcmp(argv[6], "-t") != 0) {
            cout << "Error: missing or extra arguments" << endl;
            return 1;
        }

        const char *hostname = argv[3];
        int port = atoi(argv[5]);
        int time = atoi(argv[7]);

        if (!validate_port(port)) {
            cout << "Error: port number must be in the range of [1024, 65535]" << endl;
            return 1;
        }

        if (time <= 0) {
            cout << "Error: time argument must be greater than 0" << endl;
            return 1;
        }

        run_client(hostname, port, time);
    }
    // Invalid mode
    else {
        cout << "Error: invalid mode" << endl;
        print_usage();
        return 1;
    }

    return 0;
}