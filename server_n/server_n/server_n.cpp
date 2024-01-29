#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <string>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

class FileManager {
public:
	std::vector<char> readFile(std::string& filename) {
		std::string relativePath = "s_storage\\" + filename;
		std::ifstream file(relativePath, std::ios::binary);

		if (!file.is_open()) {
			std::cerr << "\n\error\n\n";
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
		std::string relativePath = "s_storage\\" + filename;
		std::ofstream file(relativePath, std::ios::binary);
		file.write(buffer.data(), bufferSize);
	}

private:
	std::streamsize bufferSize;
};


class Server {
public:
	void initializeServer() {
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << "WSAStartup failed" << '\n';

			return;
		}

		serverSocket = socket(AF_INET, SOCK_STREAM, 0);

		if (serverSocket == INVALID_SOCKET)
		{
			std::cerr << "Error creating socket: " << WSAGetLastError() << '\n';
			WSACleanup();

			return;
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(port);
		std::cout << "\nServer intialized\n";
	}


	void bindServer() {
		if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cerr << "Bind failed with error: " << WSAGetLastError() << '\n';
			closesocket(serverSocket);
			WSACleanup();

			return;
		}
	}


	void listenToConnections() {
		if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cerr << "Listen failed with error: " << WSAGetLastError() << '\n';
			closesocket(serverSocket);
			WSACleanup();

			return;
		}
		std::cout << "\nServer listening on port " << port << '\n';
	}


	void acceptConnection() {

		clientSocket = accept(serverSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cerr << "Accept failed with error: " << WSAGetLastError() << '\n';
			closesocket(serverSocket);
			WSACleanup();

			return;
		}
	}

	void setupAndConnect() {
		initializeServer();
		bindServer();
		listenToConnections();
		acceptConnection();
	}

	void getFile(FileManager& fileManager, std::string& filename) {
		const std::vector<char> fileData = fileManager.readFile(filename);
		std::streamsize bufferSize = fileManager.getBufferSize();
		int bytesSent = send(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);

		if (WSAGetLastError() != 0) {
			std::cerr << "\nError opening file\n";
			return;
		}

		send(clientSocket, fileData.data(), (int)bufferSize, 0);
	}

	void putFile(FileManager& fileManager, std::string& filename) {
		std::streamsize bufferSize;
		int bytesReceived = recv(clientSocket, (char*)&bufferSize, sizeof(std::streamsize), 0);
		if (bytesReceived == 0) {
			const char* response = "\nError creating the file\n";
			send(clientSocket, response, (int)strlen(response), 0);

			return;
		}

		std::vector<char> buffer(bufferSize, 0);
		bytesReceived = recv(clientSocket, buffer.data(), (int)bufferSize, 0);
		if (bytesReceived > 0)
		{
			fileManager.writeFile(filename, buffer, (int)bufferSize);
			std::string strResponse = "\n\nfile " + filename + " created.\n\n";
			auto response = strResponse.c_str();
			send(clientSocket, response, (int)strlen(response), 0);
		}

		else {
			const char* response = "\nError creating the file\n";
			send(clientSocket, response, (int)strlen(response), 0);
		}
	}

	std::wstring toWideString(std::string& string) {
		int length = MultiByteToWideChar(CP_ACP, 0, string.c_str(), -1, NULL, 0);
		std::wstring wideString(length, L'\0');
		MultiByteToWideChar(CP_ACP, 0, string.c_str(), -1, &wideString[0], length);

		return wideString;
	}

	void deleteFile(std::string& filename) {
		std::string relativePath = "s_storage\\" + filename;
		std::wstring wstrRelativePath = toWideString(relativePath);
		bool isDeleted = DeleteFile(wstrRelativePath.c_str());

		if (isDeleted) {
			std::cout << "\ndeleted successfully\n";
			std::string strResponse = "\nFile " + filename + " deleted.\n";
			auto response = strResponse.c_str();
			send(clientSocket, response, (int)strlen(response), 0);
		}

		else {
			const char* response = "\nError deleting the file\n";
			send(clientSocket, response, (int)strlen(response), 0);
		}
	}

	void viewInfoFile(std::string& filename) {
		BY_HANDLE_FILE_INFORMATION fileInfo = {};
		const char* errorMsg = "\nerror\n";
		int codeOfError = 0;

		std::string relativePath = "s_storage\\" + filename;
		std::wstring wstrRelativePath = toWideString(relativePath);

		HANDLE fileHandle = CreateFile(wstrRelativePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE) {

			codeOfError = GetLastError();
			std::cerr << "File handle error code: " << codeOfError << '\n';
		}

		if (!GetFileInformationByHandle(fileHandle, &fileInfo)) {

			codeOfError = GetLastError();
			std::cerr << "file info err code: " << codeOfError << '\n';
		}

		if (codeOfError != 0) {

			send(clientSocket, errorMsg, (int)strlen(errorMsg), 0);
			return;
		}

		CloseHandle(fileHandle);

		std::stringstream fileInfoStream;
		SYSTEMTIME lastAccessTime, lastWriteTime, creationTime;
		FileTimeToSystemTime(&fileInfo.ftLastAccessTime, &lastAccessTime);
		FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &lastWriteTime);
		FileTimeToSystemTime(&fileInfo.ftCreationTime, &creationTime);

		fileInfoStream << "\ncreated on: " << creationTime.wDay << '/'
			<< creationTime.wMonth << '/' << creationTime.wYear;

		fileInfoStream << "\nlast modified: " << lastWriteTime.wDay << '/'
			<< lastWriteTime.wMonth << '/' << lastWriteTime.wYear;

		fileInfoStream << "\n\nlast accessed: " << lastAccessTime.wDay << '/'
			<< lastAccessTime.wMonth << '/' << lastAccessTime.wYear;

		

		

		LARGE_INTEGER fileSize;
		fileSize.LowPart = fileInfo.nFileSizeLow;
		fileSize.HighPart = fileInfo.nFileSizeHigh;

		float filesize = fileSize.QuadPart / 1024;

		if (filesize < 1) {

			fileInfoStream << "\nfile size: " << fileSize.QuadPart << " bytes\n";
		}
		else {

			fileInfoStream << "\nFile size: " << filesize << " kilobytes\n";
		}

		std::string fileInfoString = fileInfoStream.str();
		std::vector<char> buffer(fileInfoString.begin(), fileInfoString.end());
		send(clientSocket, buffer.data(), buffer.size(), 0);
	}

	void viewInfoList(std::string& dirname) {
		WIN32_FIND_DATA fileData;
		std::string relativePath;

		if (dirname == "-") {

			relativePath = "s_storage\\*";
		}
		else {
			relativePath = "s_storage\\" + dirname + "\\*";
		}

		std::wstring wstrRelativePath = toWideString(relativePath);
		HANDLE hFind = FindFirstFile(wstrRelativePath.c_str(), &fileData);
		std::stringstream filenames;

		if (hFind != INVALID_HANDLE_VALUE) {

			while (FindNextFile(hFind, &fileData) != 0)
			{
				std::wstring wstrFilename(fileData.cFileName);
				std::string filename(wstrFilename.begin(), wstrFilename.end());

				filenames << filename << '\n';
			}
		}

		std::string concatenatedNames = filenames.str();
		std::vector<char> buffer(concatenatedNames.begin(), concatenatedNames.end());

		send(clientSocket, buffer.data(), buffer.size(), 0);
	}

	void recieveData(FileManager& fileManager) {

		std::vector<char> buffer(1024, 0);
		int bytesReceived = recv(clientSocket, buffer.data(), buffer.size(), 0);
		if (bytesReceived > 0)
		{
			std::cout << "\nmessages |" << buffer.data() << '\n';

			std::stringstream arguments(buffer.data());
			std::string commandType, filename;

			arguments >> commandType >> filename;

			if (commandType == "PUT") {
				putFile(fileManager, filename);
			}

			else if (commandType == "GET") {
				getFile(fileManager, filename);
			}

			else if (commandType == "DELETE") {
				deleteFile(filename);
			}

			else if (commandType == "INFO") {
				viewInfoFile(filename);
			}

			else if (commandType == "LIST") {
				viewInfoList(filename);
			}

			else {
				std::cerr << "\nwrong command.\n";
			}
		}

	}


	void sendData(const char* buffer) {
		send(clientSocket, buffer, (int)strlen(buffer), 0);
	}

	~Server() {
		closesocket(clientSocket);
		closesocket(serverSocket);
		WSACleanup();
	}

private:
	WSADATA wsaData;
	int port = 12345;
	SOCKET serverSocket;
	SOCKET clientSocket;
	sockaddr_in serverAddr;
};

class Program {
public:
	void commandRunner(Server& server, FileManager& fileManager) {

		while (true) {

			server.recieveData(fileManager);
		}
	}
};

int main()
{
	Server server;
	FileManager fileManager;
	Program program;
	server.setupAndConnect();
	program.commandRunner(server, fileManager);

	return 0;
}
