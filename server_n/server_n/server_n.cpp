#include <iostream>
#include <WinSock2.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <thread> 
#include <mutex> 

#pragma comment(lib, "ws2_32.lib")

const std::string SERVER_DIR = "s_storage/";
std::mutex mtx; 

void sendFileList(SOCKET clientSocket) {
    std::vector<std::string> files;
    std::string fileList;

    WIN32_FIND_DATAA fileData;
    HANDLE hFind;
    hFind = FindFirstFileA((SERVER_DIR + "*").c_str(), &fileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                files.push_back(fileData.cFileName);
            }
        } while (FindNextFileA(hFind, &fileData));
        FindClose(hFind);
    }

    for (const auto& file : files) {
        fileList += file + "\n";
    }

    send(clientSocket, fileList.c_str(), fileList.size(), 0);
}

void sendFile(SOCKET clientSocket, const std::string& filename) {
    std::ifstream file(SERVER_DIR + filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::string error = "file does not exist";
        send(clientSocket, error.c_str(), error.size(), 0);
        return;
    }
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> fileBuffer(fileSize);
    if (file.read(fileBuffer.data(), fileSize)) {
        send(clientSocket, fileBuffer.data(), fileBuffer.size(), 0);
    }
    file.close();
}

void receiveFile(SOCKET clientSocket, const std::string& filename) {
    std::ofstream file(SERVER_DIR + filename, std::ios::binary);
    if (!file.is_open()) {
        std::string error = "error creating file";
        send(clientSocket, error.c_str(), error.size(), 0);
        return;
    }

    char buffer[1024];
    int bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        file.write(buffer, bytesRead);
    }

    file.close();
}

void deleteFile(const std::string& filename) {
    std::string filePath = SERVER_DIR + filename;
    if (remove(filePath.c_str()) != 0) {
        std::cerr << "error deleting file: " << filename << std::endl;
    }
}

void clientHandler(SOCKET clientSocket) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0)
    {
        std::cout << "received data:" << buffer << std::endl;

        std::istringstream iss(buffer);
        std::string command;
        iss >> command;

        if (command == "GET") {
            std::string filename;
            iss >> filename;
            sendFile(clientSocket, filename);
        }
        else if (command == "LIST") {
            sendFileList(clientSocket);
        }
        else if (command == "PUT") {
            std::string filename;
            iss >> filename;
            receiveFile(clientSocket, filename);
        }
        else if (command == "DELETE") {
            std::string filename;
            iss >> filename;
            deleteFile(filename);
        }
        else {
            std::string error = "Invalid command";
            send(clientSocket, error.c_str(), error.size(), 0);
        }
    }

    closesocket(clientSocket);
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "fail" << std::endl;
        return 1;
    }

    int port = 12345;
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "error creating socket" << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "bind error" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "server listening on port " << port << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        std::thread clientThread(clientHandler, clientSocket);
        clientThread.detach(); 
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
