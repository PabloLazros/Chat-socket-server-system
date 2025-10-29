#include <stdio.h>        // Standard input-output library
#include <stdlib.h>       // Standard library functions
#include <string.h>       // String manipulation functions
#include <unistd.h>       // POSIX operating system API
#include <arpa/inet.h>    // Definitions for internet operations
#include <pthread.h>      // POSIX threads library
#include <signal.h>       // Signal handling library

#define MAX_CLIENTS 10 // Maximum number of clients the server can handle
#define BUFFER_SIZE 1024 // Size of the buffer used for communication

// Structure to represent a client
typedef struct {
    int client_socket;
    char username[30];
} Client;

// Array to store client information
Client clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread synchronization
// Mutex (mutual exclution)variable for thread synchronization, initialized statically

void send_http_response(int client_socket);// Function prototype for sending HTTP responses

void recv_and_null_terminate(int sock, char *buffer, size_t buffer_size);// Function to receive data and null-terminate it
volatile sig_atomic_t server_running = 1;// Signal handler to handle termination signal
// This could be interpreted as the server initially being in a running state.

// used to indicate if the server is running
void handle_signal(int sig) {
    server_running = 0;
}

// Function to authenticate users (dummy authentication for two users)
int authenticate(char *username, char *password) {
    if (strcmp(username, "user1") == 0 && strcmp(password, "pass1") == 0) {
        return 1; // Authentication successful for user1
    } else if (strcmp(username, "user2") == 0 && strcmp(password, "pass2") == 0) {
        return 1; // Authentication successful for user2
    }
    return 0; // Authentication failed
}

// Function to check if the request is an HTTP GET request
int is_http_request(const char *buffer) {
    return strncmp(buffer, "GET", 3) == 0;
}

// Function to send a simple HTTP response
void send_http_response(int client_socket) {
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body>"
        "<h1>Welcome to the Chat Server</h1>"
        "<p>This is a simple web interface.</p>"
        "<p>Visit our chat system <a href=\"http://127.0.0.1:8080/\">here</a>.</p>"
        "</body></html>";

    send(client_socket, response, strlen(response), 0);
}

// Function to handle a WebSocket-like upgrade (simplified)
void handle_websocket_upgrade(int client_socket) {
    // A real WebSocket upgrade would require more complex handling here
    // Prepare the WebSocket upgrade response
    const char *upgrade_response = 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n\r\n";

    send(client_socket, upgrade_response, strlen(upgrade_response), 0);
    // Send the WebSocket upgrade response to the client
}

// Function to handle communication with a client
void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + 50]; // Extra space for username

    // Send a welcome message to the client
    char welcome_msg[] = "Welcome to the chat server!\n";
    send(client->client_socket, welcome_msg, sizeof(welcome_msg), 0);

while (1) {
    // Receive data from the client
    int bytes_received = recv(client->client_socket, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received] = '\0'; // Null-terminate the buffer
// to mark the end of the char
    // Check if the client sent a valid message
    if (bytes_received <= 0 || strncmp(buffer, "/exit", 5) == 0) {
        if (strncmp(buffer, "/exit", 5) == 0) {
            // Set the server_running flag to stop the server
            server_running = 0;
            
            // Broadcast a shutdown message to all clients
            pthread_mutex_lock(&mutex);  // Release the lock after finishing the critical section
//Once a thread holds the lock, it enters the critical section, and other threads attempting to
//  acquire the lock will be blocked until the lock is released using 
           
            char shutdown_msg[] = "Server is shutting down.\n";
            for (int i = 0; i < num_clients; ++i) {
                send(clients[i].client_socket, shutdown_msg, strlen(shutdown_msg), 0);
                close(clients[i].client_socket); // Close client socket
            }
            pthread_mutex_unlock(&mutex);
        }
        
        // Handle client disconnection or exit command
        pthread_mutex_lock(&mutex);
        printf("User %s disconnected.\n", client->username);
        for (int i = 0; i < num_clients; ++i) {
            if (clients[i].client_socket == client->client_socket) {
                for (int j = i; j < num_clients - 1; ++j) {
                    clients[j] = clients[j + 1];
                }
                num_clients--;
                break;
            }
  }
        pthread_mutex_unlock(&mutex);
        
        close(client->client_socket);
        free(client);
        pthread_exit(NULL);
    }

    if (is_http_request(buffer)) { 
//This condition checks whether the content of the buffer variable represents an HTTP request
        // Serve HTTP request and end the thread
        send_http_response(client->client_socket);//send an HTTP response back to the client using the socket stored
        close(client->client_socket);
        break;
    } else {
        // Handle chat messages and broadcast to other clients 
        //assumed not a http response but chat msg
        snprintf(message, sizeof(message), "%s sent: %s", client->username, buffer);
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_clients; ++i) {//checks whether the current client being iterated over is not the same client that sent
            if (clients[i].client_socket != client->client_socket) {
                send(clients[i].client_socket, message, strlen(message), 0);
            }
        }
        pthread_mutex_unlock(&mutex);
        }
    }
}


// Function to check if a username is already connected
int is_username_connected(const char *username) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_clients; ++i) {
        if (strcmp(clients[i].username, username) == 0) {
            pthread_mutex_unlock(&mutex);
            return 1; // Username is already connected
        }
    }
    pthread_mutex_unlock(&mutex);
    return 0; // Username is not connected
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Register a signal handler for Ctrl+C (SIGINT)
    signal(SIGINT, handle_signal);
// "Signal Interrupt" and is typically generated when a user presses Ctrl+C in the terminal. 
// When a program receives a SIGINT signal, it is a request for the program to interrupt 
// its execution gracefully and usually terminate.


    // Create a server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    
    // Initialize the server_addr structure with zeros
    memset(&server_addr, 0, sizeof(server_addr));

    // Set the address family to AF_INET, indicating IPv4
//stands for Internet Protocol version 4. It is the fourth version of the Internet Protocol, 
//the fundamental suite of protocols that enable communication on the internet. 
//IPv4 uses a 32-bit address scheme allowing for a total of 2^32 addresses (just over 4 billion addresses).

    server_addr.sin_family = AF_INET;

    // Set the IP address to INADDR_ANY, which allows the socket to bind to any available network interface
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Set the port number to 9999 using htons (host to network short) to ensure correct byte order
    server_addr.sin_port = htons(9999);


    // Bind the server socket to the specified address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming client connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Error listening for connections");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Server is now ready and listening for incoming connections
    printf("Server listening on port 9999...\n");

// Main loop checks the server_running flag
while (server_running) {
    if (!server_running) break; // Additional check before potentially blocking call

    // Accept incoming client connection
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    
    if (client_socket == -1) {
//   Check if an error occurred during the acceptance of a connection
        perror("Error accepting connection");
        continue; // Continue to the next iteration if there's an error
    }

    // Initial data handling
    char initial_data[BUFFER_SIZE];
    recv_and_null_terminate(client_socket, initial_data, BUFFER_SIZE);

    if (is_http_request(initial_data)) {
        // Handle HTTP request
        send_http_response(client_socket);
        close(client_socket); // Close the connection after serving the web request
        continue; // Move to the next client
    } else {
        // Improved authentication protocol
        if (strcmp(initial_data, "START_AUTH") != 0) {
            printf("Unexpected initial data from client. Closing connection.\n");
            close(client_socket);
            continue; // Move to the next client
  }

// Receive and handle username and password from the client
char username[30], password[30];
recv_and_null_terminate(client_socket, username, sizeof(username));
recv_and_null_terminate(client_socket, password, sizeof(password));

// Authenticate the client
if (!authenticate(username, password)) {
    // Authentication failed
    char auth_fail[] = "AUTH_FAILED";
    send(client_socket, auth_fail, sizeof(auth_fail), 0);
    printf("Authentication failed for user %s.\n", username);
    close(client_socket);
    continue; // Move to the next client
}

// Check if the username is already connected
if (is_username_connected(username)) {
    // User is already connected, reject duplicate connection
    char user_already_connected[] = "USER_ALREADY_CONNECTED";
    send(client_socket, user_already_connected, sizeof(user_already_connected), 0);
    printf("User %s is already connected. Rejecting duplicate connection.\n", username);
    close(client_socket);
    continue; // Move to the next client
}
// Handle successful authentication
char auth_success[] = "AUTH_SUCCESS";
send(client_socket, auth_success, sizeof(auth_success), 0);
printf("User %s connected.\n", username);

// Create a new client structure and add it to the list
Client *new_client = (Client *)malloc(sizeof(Client));
new_client->client_socket = client_socket;
strcpy(new_client->username, username);

pthread_mutex_lock(&mutex);

if (num_clients < MAX_CLIENTS) {
    // Add the new client to the clients array
    clients[num_clients++] = *new_client;
    // Create a thread to handle the new client
    pthread_t tid;
    pthread_create(&tid, NULL, handle_client, (void *)new_client);
    pthread_detach(tid);
} else {
    // Server is full, reject the connection
    printf("Server is full. Connection rejected.\n");
    close(client_socket);
    free(new_client);
}

  pthread_mutex_unlock(&mutex);
  }
}

// cleanup before server shutdown
pthread_mutex_lock(&mutex);
for (int i = 0; i < num_clients; ++i) {
    close(clients[i].client_socket); // Ensure all client sockets are closed
}
pthread_mutex_unlock(&mutex);

close(server_socket); // Close server socket
printf("Server shutdown.\n");

return 0;
}
// Function to receive data from the socket and null-terminate it
void recv_and_null_terminate(int sock, char *buffer, size_t buffer_size) {
    int bytes_received = recv(sock, buffer, buffer_size - 1, 0);

    if (bytes_received <= 0) {
        // No data received or an error occurred, null-terminate the buffer
        buffer[0] = '\0';
    } else {
        // Null-terminate the received data
        buffer[bytes_received] = '\0';
    }
}





