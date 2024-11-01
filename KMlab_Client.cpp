#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 1043; 
const std::string SERVER_IP = "127.0.0.1";
int y222;

void logAction(const std::string& action) {
    std::ofstream log("client_log.txt", std::ios::app);
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

void pingServer(SOCKET client_socket, int message_size, int interval_ms,int ping_counts = 10) {
    std::vector<char> ping_message(message_size, 'x');
    int length = htonl(static_cast<int>(ping_message.size()));

    while (ping_counts > 0) {
        auto start = std::chrono::high_resolution_clock::now();

        //PING
        send(client_socket, "PING", 4, 0);
        send(client_socket, reinterpret_cast<char*>(&length), 4, 0);
        send(client_socket, ping_message.data(), static_cast<int>(ping_message.size()), 0);
        logAction("Sent PING command");

        // PING anwser
        char header[8];
        if (!receiveData(client_socket, header, sizeof(header))) {
            std::cerr << "Failed to receive header, stopping ping." << std::endl;
            break;
        }

        std::string command(header, 4);
        int data_length = ntohl(*(int*)(header + 4));

        if (data_length > 0 && data_length <= message_size) {
            std::vector<char> response(data_length);
            if (!receiveData(client_socket, response.data(), data_length)) {
                std::cerr << "Failed to receive full response data." << std::endl;
                break;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            logAction("Received PING response, RTT = " + std::to_string(elapsed.count()) + " ms");
            std::cout << "Round Trip Time: " << elapsed.count() << " ms" << std::endl;
        }
        else {
            std::cerr << "Invalid data length received, stopping ping." << std::endl;
            break;
        }
        
        --ping_counts;

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &server_addr.sin_addr);

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed!" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    int message_size, interval_ms, ping_counts;
    std::cout << "Enter message size (bytes): ";
    std::cin >> message_size;
    std::cout << "Enter interval between pings (ms): ";
    std::cin >> interval_ms;
    std::cout << "Enter the number of pings: ";
    std::cin >> ping_counts;

    pingServer(client_socket, message_size, interval_ms, ping_counts);

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
