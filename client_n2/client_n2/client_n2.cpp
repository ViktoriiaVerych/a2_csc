#include <iostream>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

void receiveFile(SOCKET serverSocket, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error creating file" << std::endl;
        return;
    }

    char buffer[1024];
    int bytesRead;
    while ((bytesRead = recv(serverSocket, buffer, sizeof(buffer), 0)) > 0) {
        file.write(buffer, bytesRead);
    }

    file.close();
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    int port = 12345;
    PCWSTR serverIp = L"127.0.0.1";
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    InetPton(AF_INET, serverIp, &serverAddr.sin_addr);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::string command;
    std::cout << "Enter command: ";
    std::getline(std::cin, command);

    send(clientSocket, command.c_str(), static_cast<int>(command.size()), 0);

    char buffer[1024];
    memset(buffer, 0, 1024);
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0)
    {
        if (std::string(buffer) == "File does not exist") {
            std::cout << "File does not exist on server" << std::endl;
        }
        else {
            std::istringstream iss(buffer);
            std::string filename;
            iss >> filename;
            receiveFile(clientSocket, filename);
            std::cout << "File received: " << filename << std::endl;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}