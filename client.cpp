#include <iostream>
#include <cstring> // memset
#include <sys/socket.h> // socket and connect
#include <arpa/inet.h> // inet_pton
#include <unistd.h> // close read
#include <limits> // For numeric_limits

#define PORT 8080 // Port number for the server and client to connect

/*
IPv4 Socket
Socket: A combination of:
1. IP address: Identifies the device in the network
2. Port number: Identifies the application/service on the device
*/

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    /*
    Create socket for communication
    AF_INET = IPv$ address family
    SOCK_STREAM = TCP (Transmission Control Protocol)
    0 = Default protocol for TCP
    */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error" << std::endl;
        return -1;
    }

    /*
    Setting up the server address
    sin_family = Specifies IPv4
    sin_port = Converts the port number to network byte order using htons()
    */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converts the server's IP address (127.0.0.1) into a format the socket can understand (binary form).
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    /*
    Attempts to connect client socket (sock) to the server at the specified address (serv_addr)

    1st param: Socket file descriptor
    2nd param: The server's address (cast to sockaddr)
    3rd param: The size of the serv_addr
    */
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection Failed" << std::endl;
        return -1;
    }

    std::cout << "Connected to server!" << std::endl;

    // Start the game loop
    while (true) {
        /*
        Resets the contents of buffer (a character array)
        Just to make sure that no leftover data remains in the buffer
        */
        memset(buffer, 0, sizeof(buffer));

        /*
        Reads data sent by client into the buffer.
        1kb is 1024 bytes, standard size for small text-based communications between client and server
        */ 
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            // Uses strcmp to compare two C-style strings, will result 0 if two strings are equal
            if (strcmp(buffer, "Server (X) wins!") == 0 || strcmp(buffer, "Client (O) wins!") == 0 || strcmp(buffer, "Draw") == 0) {
                std::cout << buffer << std::endl;
                break;  // End the game if there's a result
            }

            std::cout << "Server: Here is the board:" << std::endl;

            // Display the updated board
            for (int i = 0; i < 9; i++) {
                std::cout << buffer[i] << " ";
                if ((i + 1) % 3 == 0) std::cout << std::endl;
            }

            int clientChoice;

            // Only allow input if the game isn't over
            while (true) {
                std::cout << "Your turn (O), choose a number (1-9): ";
                std::cin >> clientChoice;

                // Validate the input
                if (std::cin.fail()) {
                    std::cin.clear(); // Clear the error flag
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
                    std::cout << "Invalid input. Please enter a number between 1 and 9." << std::endl;
                    continue;
                }

                if (clientChoice < 1 || clientChoice > 9) {
                    std::cout << "Invalid choice. Try again." << std::endl;
                    continue;
                }

                // Check if the chosen position is available
                if (buffer[clientChoice - 1] == 'X' || buffer[clientChoice - 1] == 'O') {
                    std::cout << "Position already occupied. Choose another number." << std::endl;
                    continue;
                }

                break; // Break out of the input loop
            }

            
                /*
                1st param: Socket descriptor containing necessary data for client's connection to the server
                2nd param: Converts int to string. Uses c_str() since send() requires a character array or pointer. Need to convert it to string since send() works with raw byte data
                3rd param: Tells send() how many bytes it should send
                4th param: Indicates no special flags. Meaning, it's a simple blocking send operation
                */
                send(sock, std::to_string(clientChoice).c_str(), strlen(std::to_string(clientChoice).c_str()), 0);
        }
    }

    close(sock);
    return 0;
}
