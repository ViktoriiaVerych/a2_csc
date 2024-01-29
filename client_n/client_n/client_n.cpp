#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <WinSock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class FileManager {
public:
    std::vector<char> readFile(std::string& filename) {
        std::string relativePath = "c_storage\\" + filename;
        std::ifstream file(relativePath, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "\n\nerror opening file\n\n";
            std::vector<char> empty;
            return empty;
        }

        file.seekg(0, std::ios::end);
        bufferSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(bufferSize, 0);
        file.read(buffer.data(), bufferSize);
        file.close();

        return buffer;
    }

    const std::streamsize getBufferSize() {
        return bufferSize;
    }

    void writeFile(std::string& filename, std::vector<char>& buffer, int bufferSize) {
        std::string relativePath = "c_storage\\" + filename;
        std::ofstream file(relativePath, std::ios::binary);
        file.write(buffer.data(), bufferSize);
    }

private:
    std::streamsize bufferSize;
};


class CS1 {
public:
    CS1(PCWSTR Ip)
        : serverIp(Ip) {}

    void connectServer() {

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cerr << "WSAStartup failed" << '\n';

            return;
        }

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Error creating socket: " << WSAGetLastError() << '\n';
            WSACleanup();

            return;
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        InetPton(AF_INET, serverIp, &serverAddr.sin_addr);


        if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
        {
            std::cerr << "Connect failed with error: " << WSAGetLastError() << '\n';
            closesocket(clientSocket);
            WSACleanup();

            return;
        }

    }


    void sendArgs(const char* message) {

        send(clientSocket, message, (int)strlen(message), 0);
    }


    void putFile(FileManager& fileManager, std::string& filename) {

        const std::vector<char> fileData = fileManager.readFile(filename);

        auto bufferSize = fileManager.getBufferSize();
        send(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);

        send(clientSocket, fileData.data(), (int)bufferSize, 0);

        std::vector<char> confirmationBuffer(128, 0);
        int bytesReceived = recv(clientSocket, confirmationBuffer.data(), confirmationBuffer.size(), 0);
        if (bytesReceived > 0) {

            std::cout << confirmationBuffer.data();
        }
    }


    void receiveData(std::string& filename, FileManager& fileManager) {

        std::streamsize bufferSize;
        int bytesReceived = recv(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);
        if (bytesReceived == 0) {

            std::cerr << "\nError getting file.\n";
            return;
        }

        std::vector<char> buffer(bufferSize, 0);

        bytesReceived = recv(clientSocket, buffer.data(), (int)bufferSize, 0);
        if (bytesReceived > 0)
        {

            fileManager.writeFile(filename, buffer, (int)bufferSize);

            std::cout << "File created.\n";
        }
    }

    void recieveMes() {

        std::vector<char> buffer(128, 0);
        recv(clientSocket, buffer.data(), buffer.size(), 0);

        std::cout << buffer.data();

    }

    void recieveFileInfo() {

        std::vector<char> buffer(256, 0);

        int bytesReceived = recv(clientSocket, buffer.data(), buffer.size(), 0);
        if (bytesReceived > 0) {

            std::cout << buffer.data();
        }
        else {

            std::cout << "\nFile not found.\n";
        }
    }

    void recieveInfoList() {

        std::vector<char> buffer(1024, 0);
        recv(clientSocket, buffer.data(), buffer.size(), 0);

        std::cout << '\n' << buffer.data() << "\n";
    }

    SOCKET& getSocket() {
        return clientSocket;
    }

    sockaddr_in getAddress() {
        return serverAddr;
    }

    ~CS1() {
        closesocket(clientSocket);
        WSACleanup();
    }

private:
    WSADATA wsaData;
    PCWSTR serverIp;
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    int port = 12345;

};

class Runner {
public:
    void commandRunner(CS1& server, FileManager& fileManager) {
        while (true) {

            std::cout << "\nenter the command (PUT, GET, DELETE, INFO, LIST): ";
            std::cin >> commandType;
            std::cin.ignore();
            std::cout << "\nenter the path: ";
            std::cin >> filename;
            std::string userInput = commandType + " " + filename;
            const char* response = userInput.c_str();
            server.sendArgs(response);

            if (commandType == "PUT") {
                server.putFile(fileManager, filename);
            }

            else if (commandType == "GET") {
                server.receiveData(filename, fileManager);
            }

            else if (commandType == "DELETE") {
                server.recieveMes();
            }

            else if (commandType == "INFO") {
                server.recieveFileInfo();
            }

            else if (commandType == "LIST") {
                server.recieveInfoList();
            }
        }
    }

    const std::string getFileName() {
        return filename;
    }

private:
    std::string commandType;
    std::string filename;
};

int main()
{
    CS1 server(L"127.0.0.1");
    Runner Commands;
    FileManager fileManager;
    server.connectServer();
    Commands.commandRunner(server, fileManager);

    return 0;
}
