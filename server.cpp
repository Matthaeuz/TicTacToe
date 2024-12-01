#include <iostream>
#include <cstring> // memset
#include <sys/socket.h> // socket and connect
#include <netinet/in.h>
#include <unistd.h> // close read
#include <limits> // For numeric_limits

#define PORT 8080 // Port number for the server and client to connect

// Function to display the Tic Tac Toe board
void displayBoard(char board[3][3]) {
    std::cout << "Current Board:" << std::endl;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            std::cout << board[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

// Function to check if the player has won
bool checkWin(char board[3][3], char player) {
    // For horizontal and vertical
    for (int i = 0; i < 3; ++i) {
        if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) || 
            (board[0][i] == player && board[1][i] == player && board[2][i] == player)) {
            return true;
        }
    }
    // For diagonal
    if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) || 
        (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
        return true;
    }
    return false;
}

// Function to check if it's a draw
bool checkDraw(char board[3][3]) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] != 'X' && board[i][j] != 'O') {
                return false; // Empty cell means no draws yet
            }
        }
    }
    return true; // No empty cells left means it's a draw
}

int main() {
    /*
    server_fd will be the File Descriptor for the server pocket.
    In Unix-ike operating systems, since everything (including sockets) are treated as a file,
    we will be using server_fd to interact with the operating system to perform operations
    on the socket (like binding, listening, and accepting connections)
    */
    int server_fd = 0, new_socket = 0;
    /*
    struct sockaddr_in is a structure that defines the Internet address for an IPv4 socket
    It contains the fields sin_family, sin_port, and sin_addr.s_addr.
    sin_family = address family (AF_INET for IPv4)
    sin_port = port number
    sin_addr.s_addr = IP Address. INADDR_ANY for accepting connections from any IP
    */
    struct sockaddr_in address;
    /*
    Used to store the size of the address stucture (sockaddr_in) in bytes
    since both bind() and accept() will be needing the size passed to them as value
    */
    int addrlen = sizeof(address);
    char buffer[1024] = {0}; // Used to store data received from the client or to send data to the client

    /*
    This section creates a socket so clients know where to connect
    AF_INET = the address family IPv4
    SOCK_STREAM = TCP (Transmission Control Protocol)
    Returns a file descriptor server_fd representing the socket
    */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return -1;
    }

    // Bind socket to the port
    address.sin_family = AF_INET; // IPv4
    address.sin_port = htons(PORT); // htons() ensures that the port number is in correct format for communication
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP

    /*
    Links server_fd to the specified IP and port

    1st param: The file descriptor of the server's socket, it identifies which socket you want to bind
    2nd param: The address information containing the IP and Port where the server will listen for connections. Uses struct sockaddr * since bind() expects a generic type
    3rd param: Tells bind() how much memory to read for the address
    */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }

    /*
    Tells the OS that this socket is now listening socket
    3 means the server can have up to 3 pending connection requests in the queue
    */
    if (listen(server_fd, 3) < 0) { 
        perror("Listen failed");
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << std::endl;

    /*
    Accept() waits for a client to connect
    Once a client connects, it creates a new socket new_socket for that connection
    The original server_fd continues to listen for more connections

    2nd param: The address information containing the IP and Port where the server will listen for connections
    3rd param: Uses socklen_t data type since it is specifically used for sizes of socket-related structures. 
    And since accept() also expects the third parameter to be a pointer to a socklen_t data type
    */
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        return -1;
    }

    // Initialize the board
    char board[3][3] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}};

    bool serverTurn = true;  // Server goes first
    bool isInitial = true;

    // Start the game loop
    while (true) {
        // Server's turn (X)
        if (serverTurn) {
            int serverChoice;
            // Display the initial board
            if (isInitial) {
                displayBoard(board);
                isInitial = false;
            }

            std::cout << "Server (X), choose a number (1-9): ";
            std::cin >> serverChoice;

            // Validate the input
            if (std::cin.fail()) {
                std::cin.clear(); // Clear the error flag
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
                std::cout << "Invalid input. Please enter a number between 1 and 9." << std::endl;
                continue;
            }
            
            if (serverChoice < 1 || serverChoice > 9) {
                std::cout << "Invalid choice. Try again." << std::endl;
                continue;
            }

            int row = (serverChoice - 1) / 3;
            int col = (serverChoice - 1) % 3;

            if (board[row][col] != 'X' && board[row][col] != 'O') {
                board[row][col] = 'X';
                serverTurn = false;  // Switch turn to client

                // Check for win or draw
                if (checkWin(board, 'X')) {
                    std::cout << "Server (X) wins!" << std::endl;
                    send(new_socket, "Server (X) wins!", 17, 0);  // Send result to client
                    break;
                } else if (checkDraw(board)) {
                    std::cout << "It's a draw!" << std::endl;
                    send(new_socket, "Draw", 4, 0);  // Send draw to client
                    break;
                }
                
                send(new_socket, board, sizeof(board), 0);  // Send the updated board to the client
            } else {
                std::cout << "Cell already taken. Choose another." << std::endl;
            }
        }
        // Client's turn (O)
        else {
            /*
            Resets the contents of buffer (a character array)
            Just to make sure that no leftover data remains in the buffer
            */
            memset(buffer, 0, sizeof(buffer));

            /*
            Reads data sent by client into the buffer.
            1kb is 1024 bytes, standard size for small text-based communications between client and server
            */ 
            int valread = read(new_socket, buffer, 1024);
            if (valread > 0) {
                // Converts string to int since client sends data as string
                int clientChoice = atoi(buffer);
                int row = (clientChoice - 1) / 3;
                int col = (clientChoice - 1) % 3;
                if (board[row][col] != 'X' && board[row][col] != 'O') {
                    board[row][col] = 'O';
                    serverTurn = true;  // Switch turn to server
                    // Display the updated board after client's move
                    displayBoard(board);

                    // Check for win or draw
                    if (checkWin(board, 'O')) {
                        std::cout << "Client (O) wins!" << std::endl;
                        send(new_socket, "Client (O) wins!", 17, 0);  // Send result to client
                        break;
                    } else if (checkDraw(board)) {
                        std::cout << "Draw" << std::endl;
                        send(new_socket, "Draw", 4, 0);  // Send draw to client
                        break;
                    }
                } else {
                    std::cout << "Cell already taken. Client, choose another." << std::endl;
                }
            }
        }
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
