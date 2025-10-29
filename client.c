#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define LOG_FILE "chat_history.txt"
#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"

// Function to get the current date and time as a string
void getDateTime(char *dateTime) {
    time_t rawtime;
    struct tm *info;

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(dateTime, 20, "%Y-%m-%d %H:%M:%S", info);
}

FILE *log_file = NULL;

// Thread function to receive messages from the server
void *receive_messages(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    char dateTime[20];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            getDateTime(dateTime);
            printf("\n[Message Received] %s %s\n", dateTime, buffer);

            // Writing to log file
            if (log_file != NULL) {
                fprintf(log_file, "%s %s\n", dateTime, buffer);
                fflush(log_file); // Ensure it's written immediately
            }
        } else if (bytes_received == 0) {
            getDateTime(dateTime);
            printf("\r[Notification!] %s Server disconnected. Exiting...\n", dateTime);
            break;
        } else {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("recv failed");
                break;
            }
        }
    }

    return NULL;
}

// Function to open the chat interface in the browser
void access_chat_via_browser() {
    char command[100];
    sprintf(command, "xdg-open http://%s:%d", SERVER_IP, SERVER_PORT);
    system(command);
}

// Function to open the chat history log file
void open_chat_history() {
    char command[100];
    sprintf(command, "xdg-open %s", LOG_FILE);
    system(command);
}

int main() {
    unsetenv("GTK_MODULES"); // Unset GTK_MODULES to suppress the Gtk-Message
    signal(SIGPIPE, SIG_IGN);

    int client_socket;
    struct sockaddr_in server_addr;

    // Open log file
    log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set socket to blocking mode
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Attempt to connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (errno != EINPROGRESS) {
            perror("Error connecting to server");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(client_socket, &write_fds);

        struct timeval timeout;
        timeout.tv_sec = 5;  // Timeout after 5 seconds
        timeout.tv_usec = 0;

        if (select(client_socket + 1, NULL, &write_fds, NULL, &timeout) <= 0) {
            perror("Connection timeout or error");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }
    
    // Display welcome header
    printf("\n======================================================\n");
    printf(" Welcome to EchoTalk: Real-Time Chat Server and Client\n");
    printf("======================================================\n");
    
    // Authentication process
    printf("\n[Authentication]\n");
    char start_auth_msg[] = "START_AUTH";
    send(client_socket, start_auth_msg, sizeof(start_auth_msg), 0);

    char username[30], password[30], auth_response[BUFFER_SIZE];
    printf("\nEnter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    printf("Enter your password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    send(client_socket, username, strlen(username) + 1, 0);
    usleep(100000); // Small delay to ensure sequential delivery
    send(client_socket, password, strlen(password) + 1, 0);

    // Receive authentication response
    memset(auth_response, 0, sizeof(auth_response));
    if (recv(client_socket, auth_response, sizeof(auth_response), 0) <= 0) {
        perror("Failed to receive authentication response");
        close(client_socket);
        return 1; // Exit on error
    }
   
    // Check server response and act accordingly
    if (strcmp(auth_response, "AUTH_FAILED") == 0) {
        printf("Authentication unsuccessful. Exiting...\n");
        close(client_socket);
        return 1; // Exit on authentication failure
    } else if (strcmp(auth_response, "USER_ALREADY_CONNECTED") == 0) {
        printf("Username already in use. Exiting...\n");
        close(client_socket);
        return 1; // Exit on username already in use
    }

    printf("Authentication successful.\n");
    
    // Proceed with message receiving in a new thread
    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_messages, (void *)&client_socket);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK); // Re-enable non-blocking mode

    char buffer[BUFFER_SIZE];
    int user_choice;
    fd_set read_fds;
    struct timeval timeout;

while (1) {
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds); // Listen for user input
    FD_SET(client_socket, &read_fds); // Listen for incoming messages

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; // Polling interval

    if (select(client_socket + 1, &read_fds, NULL, NULL, &timeout) > 0) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            printf("\n%s\n", buffer);
            continue;
        }
    }

        printf("\n[Main Menu]\n");
        printf("  1 - Send Message\n");
        printf("  2 - View in Browser\n");
        printf("  3 - Open Chat History\n");
        printf("  4 - Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &user_choice) == 1) {
            getchar(); // Clear the newline character
        
        switch (user_choice) {
                case 1: // Send message
                    printf("Enter your message: ");
                    memset(buffer, 0, BUFFER_SIZE);
                    fgets(buffer, sizeof(buffer), stdin);
                    buffer[strcspn(buffer, "\n")] = '\0';

                    if (strcmp(buffer, "/exit") == 0) {
                        printf("Exiting chat...\n");
                        goto exit_loop;
                    }


                    // Sending message to the server
                    int total_sent = 0;
                    int length = strlen(buffer);
                    while (total_sent < length) {
                        int sent = send(client_socket, buffer + total_sent, length - total_sent, 0);
                        if (sent == -1) {
                            perror("send failed");
                            exit(EXIT_FAILURE);
                        }
                        total_sent += sent;
                    }
                    break;
                case 2: // Open chat in browser
                    access_chat_via_browser();
                    break;
                case 3: // Open chat history
                    open_chat_history();
                    break;
                case 4: // Exit
                    printf("Exiting...\n");
                    goto exit_loop;
                default:
                    printf("Invalid choice, please try again.\n");
            }
        } else {
            printf("Invalid input, please try again.\n");
            while (getchar() != '\n'); // Clear the buffer
        }
    }

exit_loop:
    if (log_file != NULL) {
        fclose(log_file); // Close the log file
    }
    close(client_socket);
    pthread_join(recv_tid, NULL); // Wait for the receive thread to finish
    return 0;
}
