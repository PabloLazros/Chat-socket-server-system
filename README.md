# EchoTalk : Real-Time Chat Server and Client

## Creators :

- Ahmed Suhail
- Wasim Chami
- Yahya Elnamla

## Project Overview

EchoTalk is a real-time chat application consisting of a client program (`client.c`) and a server program (`server.c`). Developed by Ahmed Suhail, Wasim Chami, and Yahya Elnamla, this project enables users to connect, authenticate, and engage in live chat sessions. The server efficiently manages multiple client connections, providing a seamless and interactive chatting experience.

## Client Program (`client.c`)

### Dependencies
- Standard C libraries (`stdio.h`, `stdlib.h`, `string.h`, etc.)
- POSIX threads (`pthread.h`)

### Usage
1. Compile: `gcc client.c -o client -lpthread`
2. Run: `./client`

### Features

- **Authentication:** Users must provide a username and password to access the chat.
- **Real-time Chat:** Enjoy instant messaging with other connected users.
- **View in Browser:** Access the chat interface through a web browser.
- **Chat History:** Open and view the chat history in the default text editor.

## Server Program (`server.c`)

### Dependencies
- Standard C libraries (`stdio.h`, `stdlib.h`, `string.h`, etc.)
- POSIX threads (`pthread.h`)

### Usage
1. Compile: `gcc server.c -o server -lpthread`
2. Run: `./server`

### Features
- **Multiple Clients:** Efficiently manages and handles up to 10 concurrent client connections.
- **WebSocket-Like Upgrade:** Supports a simple HTTP upgrade for web clients.
- **Authentication:** Validates users based on predefined username-password pairs.
- **HTTP Response:** Responds with a welcome message when accessed via HTTP.

## Implementation Details

- **Port Configuration:** The chat server operates on port 9999, and the server IP is set to "127.0.0.1" in the client program. Adjust these values as needed.
- **Concurrency:** The server supports a maximum of 10 concurrent clients, ensuring efficient communication.
- **Authentication:** For demonstration purposes, authentication credentials are hardcoded in the server. Modify the `authenticate` function for a real authentication mechanism.

## Security Considerations

- **Disclaimer:** This is a basic example and may not be suitable for production use without additional security measures.
- **Recommendations:** Implement encryption, secure authentication, and robust error handling for enhanced security in a production-ready chat application.

Feel free to explore and enhance EchoTalk for your specific requirements.

