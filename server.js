const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const fs = require('fs');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

const port = 8080;
const hostname = '127.0.0.1';

// Serve static files from the public directory
app.use(express.static(path.join(__dirname, 'public')));

// Redirect all traffic to index.html
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

// Chat history file path
const chatHistoryPath = path.join(__dirname, 'chat_history.txt');

// Users object for tracking online users
let onlineUsers = {};

// Socket.IO events
io.on('connection', (socket) => {
    console.log('A user connected');

    socket.on('login', (userData) => {
        const { username, password } = userData;
        // Example user validation (replace with database or other validation logic)
        const users = {
            user1: 'pass1',
            user2: 'pass2',
            // Extend with more users as needed
        };

        if (users[username] && users[username] === password) {
            onlineUsers[socket.id] = username; // Track user as online
            socket.emit('login_success', { username });

            // Send chat history to only the user who just logged in
            fs.readFile(chatHistoryPath, 'utf8', (err, data) => {
                if (err) {
                    console.log("Error reading chat history:", err);
                } else {
                    socket.emit('load history', data);
                }
            });
        } else {
            socket.emit('login_failure');
        }
    });

    // Handle chat message
    socket.on('chat message', (msg) => {
        const user = onlineUsers[socket.id];
        if (user) {
            const messageWithUser = `${user}: ${msg}`;
            // Append message to chat history
            fs.appendFile(chatHistoryPath, messageWithUser + '\n', (err) => {
                if (err) throw err;
                console.log('Message saved to history');
            });

            // Broadcast message to all clients including the sender
            io.emit('chat message', messageWithUser);
        }
    });

    socket.on('disconnect', () => {
        console.log(`${onlineUsers[socket.id] || 'A user'} disconnected`);
        delete onlineUsers[socket.id]; // Remove user from online list
    });
});

server.listen(port, hostname, () => {
    console.log(`Server running at http://${hostname}:${port}/`);
});

