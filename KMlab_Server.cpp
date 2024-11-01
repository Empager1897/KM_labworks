#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 1043;
int h222;

void logAction(const std::string& action) {
    std::ofstream log("server_log.txt", std::ios::app);
    std::time_t now = std::time(0);
    log << std::ctime(&now) << " - " << action << std::endl;
}

bool receiveData(SOCKET socket, char* buffer, int length) {
    int total_received = 0;
    while (total_received < length) {
        int bytes = recv(socket, buffer + total_received, length - total_received, 0);
        if (bytes <= 0) return false;  // Error or connection lost
        total_received += bytes;
    }
    return true;
}

void handleClient(SOCKET client_socket) {
    char header[8];
    while (true) {
        
        if (!receiveData(client_socket, header, sizeof(header))) {
            logAction("Failed to receive header, closing connection.");
            break;
        }

        std::string command(header, 4);
        int data_length = ntohl(*(int*)(header + 4));

        if (data_length > 0 && data_length < 10000) {
            std::vector<char> data(data_length);
            if (!receiveData(client_socket, data.data(), data_length)) {
                logAction("Failed to receive data, closing connection.");
                break;
            }

            if (command == "WHO ") {
                std::string response = "Author:Petro Nyvchyk";
                int length = htonl(static_cast<int>(response.size()));
                send(client_socket, "DATA", 4, 0);
                send(client_socket, reinterpret_cast<char*>(&length), 4, 0);
                send(client_socket, response.c_str(), static_cast<int>(response.size()), 0);
                logAction("Sent WHO response");
            }
            else if (command == "PING") {
                int length = htonl(data_length);
                send(client_socket, "PING", 4, 0);
                send(client_socket, reinterpret_cast<char*>(&length), 4, 0);
                send(client_socket, data.data(), data_length, 0);
                logAction("Received and sent PING response");
            }
            else if (command == "END ") {
                logAction("Connection closed by client");
                break;
            }
            else {
                std::string error_msg = "ERROR: Unknown command";
                int length = htonl(static_cast<int>(error_msg.size()));
                send(client_socket, "ERRO", 4, 0);
                send(client_socket, reinterpret_cast<char*>(&length), 4, 0);
                send(client_socket, error_msg.c_str(), static_cast<int>(error_msg.size()), 0);
                logAction("Sent ERROR response");
            }
        }
        else {
            logAction("Invalid data length received, closing connection.");
            break;
        }
    }
    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    listen(server_socket, 5);
    std::cout << "Server is running and waiting for connections..." << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed!" << std::endl;
            continue;
        }
        logAction("New client connected");
        handleClient(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
