// ���������� �������������� � ����� 4996
#pragma warning(disable : 4996)
// ��������� ���������� ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

// ����������� ����������� ������������ ������
#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <ranges>

// ������������� ����������� ���� std � filesystem
using namespace std;
using namespace filesystem;

// ����������� ������������� ������� ������
const int MAX_BUFFER_SIZE = 8192;
// �������� ������� ������� �������
unordered_map<SOCKET, string> clientSockets;
// ������� ��� ������������� ������� � ��������� �������
mutex clientSocketsMutex;

// ������� ��� ���������� �������, ���������� �� �������
string executeCommand(const string& command, SOCKET clientSocket)
{
	// ���������� ��� �������� ���������� ���������� �������
	string result;

	// ������ ��� �������� ������� �������
	vector<string> tokens;
	// �������� ������ ����� �� ������ �������
	istringstream iss(command);
	// ���������� ��� �������� �������� ������
	string token;

	// �������� ������� ������� mPUT � �������
	if (command.find("mPUT|") != string::npos) {
		// ��������� ������� �� ������ � �������������� ����������� '|'
		while (getline(iss, token, '|'))
		{
			tokens.push_back(token);
		}
	}
	else {
		// ��������� ������� �� ������ � �������������� ����������� ' '
		while (getline(iss, token, ' '))
		{
			tokens.push_back(token);
		}
	}

	// ����� ���������� � ���������� ������� � ����������� �� ���������� �������
	if (tokens.size() == 1)
	{
		cout << tokens[0] << endl;
	}
	else if (tokens.size() == 2)
	{
		cout << tokens[0] << ' ' << tokens[1] << endl;
	}
	else if (tokens.size() == 4)
	{
		cout << tokens[0] << ' ' << tokens[1] << ' ' << tokens[2] << endl;
	}
	else
	{
		cout << "���������� ���������� ���������� �������" << endl;
	}

	// ��������� ������� ls
	if (tokens[0] == "ls")
	{
		// �������� ������������ ������� ������� ls
		if (tokens.size() != 1)
		{
			result = "������: ������������ ������ ������� ls\n";
		}
		else
		{
			// ��������� �������� ���� ��������
			string currentPath = filesystem::current_path().string();
			result = "���������� �������� ��������:\n";

			// ������� ����������� �������� �������� � ���������� � ���������
			for (const auto& entry : filesystem::directory_iterator(currentPath))
			{
				result += entry.path().filename().string() + "\n";
			}
		}
	}
	// ��������� ������� mPUT
	else if (tokens[0] == "mPUT")
	{
		// �������� ������������ ������� ������� mPUT
		if (tokens.size() != 4)
		{
			result = "������: ������������ ������ ������� mPUT\n";
		}
		else
		{
			// ��������� ����� �����, ������� ����� � ������ ����� �� �������
			string fileName = tokens[1];
			int fileSize = stoi(tokens[2]);
			string fileData = tokens[3];

			// �������� ����� ��� ������ � �������� ������
			ofstream file(fileName, ios::binary);
			if (!file.is_open())
			{
				result = "������ ��� �������� ����� �� �������\n";
			}
			else
			{
				// ������ ������ ����� � ���� �� �������
				file.write(fileData.c_str(), fileSize);
				file.close();
				result = "���� ������� ���������� �� ������\n";
			}
		}
	}
	// ��������� ������� pwd
	else if (tokens[0] == "pwd")
	{
		// �������� ������������ ������� ������� pwd
		if (tokens.size() != 1)
		{
			result = "������: ������������ ������ ������� pwd\n";
		}
		else
		{
			// ��������� �������� ���� ��������
			string currentPath = filesystem::current_path().string();
			result = "������� ���� ��������: " + currentPath + "\n";
		}
	}
	// ��������� ������� bye
	else if (tokens[0] == "bye")
	{
		// �������� ������������ ������� ������� bye
		if (tokens.size() != 1)
		{
			result = "������: ������������ ������ ������� bye\n";
		}
		else
		{
			result = "���������� �� �������...\n";
		}
	}
	// ��������� ������� help
	else if (tokens[0] == "help")
	{
		// �������� ������������ ������� ������� help
		if (tokens.size() > 2)
		{
			result = "������: ������������ ������ ������� help\n";
		}
		else
		{
			// ����� ������ ��������� ������, ���� ������� help ��� ����������
			if (tokens.size() == 1)
			{
				result = "������ ��������� ������: ls, pwd, mPUT, bye, help\n"
					"\t   ��� ��������� �������������� ���������� �� ������ �� ������ ������� 'help <�������>'\n";
			}
			else
			{
				// ����� �������� ���������� �������, ���� ������ ��������
				if (tokens[1] == "ls")
				{
					result = "����� ����������� �������� �������� �� �������\n";
				}
				else if (tokens[1] == "pwd")
				{
					result = "����� ���� ������� ����������� �� �������\n";
				}
				else if (tokens[1] == "mPUT")
				{
					result = "����������� ����� � ���������� ���������� �� ������\n"
						"\t   ������ mPUT <���_�����/����_�����>\n";
				}
				else if (tokens[1] == "bye")
				{
					result = "���������� ������ � ��������\n";
				}
				else if (tokens[1] == "help")
				{
					result = "����� ���������� � ��������� ��������\n";
				}
				else
				{
					result = "������: ������������ ������ ������� help\n";
				}
			}
		}
	}
	// ��������� ����������� �������
	else
	{
		result = "����������� �������\n";
	}

	// ������� ���������� ���������� �������
	return result;
}

// ������� ��� ��������� ����������� �����������
void clientHandler(SOCKET clientSocket)
{
	// ����� ��� ������ ������ �� �������
	char buffer[MAX_BUFFER_SIZE];

	// ��������� ��� �������� ���������� �� ������ �������
	sockaddr_in clientAddress{};
	int clientAddressSize = sizeof(clientAddress);
	// ��������� ���������� �� ������ �������
	getpeername(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
	// ��������� IP-������ �������
	string clientIP = inet_ntoa(clientAddress.sin_addr);
	// ��������� ����� �������
	int clientPort = ntohs(clientAddress.sin_port);

	// ����� ���������� � ����������� �������
	cout << "������ ��������� [" << clientIP << ":" << clientPort << "]" << endl;

	// ���� ��������� ��������� �� �������
	while (true)
	{
		// ��������� ������ �� �������
		int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
		// �������� �� ������ ��� ���������� �������
		if (bytesRead == SOCKET_ERROR || bytesRead == 0)
		{
			// ���������� �������� ��� ������� � ��������� �������
			lock_guard<mutex> lock(clientSocketsMutex);
			// �������� ������ ������� �� ���������
			clientSockets.erase(clientSocket);
			// ����� ���������� �� ���������� �������
			cout << "������ �������� [" << clientIP << ":" << clientPort << "]" << endl;
			break;
		}

		// �������������� ���������� ������ � ������
		string clientMessage(buffer, bytesRead);
		// ����� ���������� � ���������� ��������� �� �������
		cout << "�� ������� [" << clientIP << ":" << clientPort << "] - ";

		// ���������� �������, ���������� �� �������
		string result = executeCommand(clientMessage, clientSocket);

		// ���������� �������� ��� ������� � ��������� �������
		{
			lock_guard<mutex> lock(clientSocketsMutex);
			// ���������� ���������� ���������� ������� ��� �������� �������
			clientSockets[clientSocket] = result;
		}
	}

	// �������� ������ �������
	closesocket(clientSocket);
}

// ������� ��� �������� ���������� ���������� ������� �������
void sendResultToClient(SOCKET clientSocket)
{
	// ����������� ���� �������� ����������� �������
	while (true)
	{
		// ���������� ��� �������� ���������� ���������� �������
		string result;

		// ���������� �������� ��� ������� � ��������� �������
		{
			lock_guard<mutex> lock(clientSocketsMutex);
			// �������� ������� ���������� ��� ������� ������� � ���������
			if (clientSockets.find(clientSocket) != clientSockets.end())
			{
				// ��������� ���������� ���������� ������� ��� ������� �������
				result = clientSockets[clientSocket];
				// �������� ���������� �� ���������
				clientSockets.erase(clientSocket);
			}
		}

		// �������� ���������� �������, ���� �� �� ������
		if (!result.empty())
		{
			send(clientSocket, result.c_str(), result.length(), 0);
		}

		// �������� ����� ��������� ��������� �����
		this_thread::sleep_for(chrono::milliseconds(100));
	}
}

// ������� ������� ���������
int main()
{
	// ��������� ������� ������
	setlocale(LC_ALL, "ru");

	// ������������� Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cerr << "������ ������������� Winsock" << endl;
		return 1;
	}

	// �������� ���������� ������
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		cerr << "������ �������� ������" << endl;
		WSACleanup();
		return 1;
	}

	// ��������� ������ �������
	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(8080);

	// �������� ���������� ������ � ������
	if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cerr << "������ ��� �������� ������ � ������" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// ������� ���������� ������ � ����� �������������
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "������ ��� ������������� �������� �����������" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// ����� ���������� � ������� �������
	cout << "������ �������. �������� �����������..." << endl;

	// ����������� ���� ��������� �������� �����������
	while (true)
	{
		// �������� ��������� �����������
		SOCKET clientSocket = accept(serverSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "������ ��� �������� ��������� �����������" << endl;
			closesocket(serverSocket);
			WSACleanup();
			return 1;
		}

		// �������� ������ ��� ��������� ����������� �����������
		thread clientThread(clientHandler, clientSocket);
		clientThread.detach();

		// �������� ������ ��� �������� ����������� �������
		thread sendThread(sendResultToClient, clientSocket);
		sendThread.detach();
	}

	// �������� ���������� ������
	closesocket(serverSocket);
	// ������� Winsock
	WSACleanup();

	// ���������� ���������
	return 0;
}
