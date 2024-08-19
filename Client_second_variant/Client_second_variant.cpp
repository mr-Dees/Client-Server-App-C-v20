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

// ������������� ����������� ���� std � filesystem
using namespace std;
using namespace filesystem;

// ����������� ������������� ������� ������
const int MAX_BUFFER_SIZE = 8192;

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

	// �������� ����������� ������
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		cerr << "������ �������� ������" << endl;
		WSACleanup();
		return 1;
	}

	// ��������� ������ �������
	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("172.20.10.4");
	serverAddress.sin_port = htons(8080);

	// ����������� � �������
	if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cerr << "������ ����������� � �������" << endl;
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	// ����������� ���� ��������� ������
	while (true)
	{
		// ���� ������� � ����������
		cout << "������� �������: ";
		string command;
		getline(cin, command);

		// �������� �� ������� mPUT (����� help)
		if (strstr(command.c_str(), "mPUT") and !strstr(command.c_str(), "help"))
		{
			// ��������� ������� �� ������
			vector<string> tokens;
			istringstream iss(command);
			string token;
			while (getline(iss, token, ' '))
			{
				tokens.push_back(token);
			}
			// �������� ������������ ������� ������� mPUT
			if (tokens.size() != 2)
			{
				cout << "������: ������������ ������ ������� mPUT\n" << endl;
				continue;
			}

			// �������� ����� ��� ������ � �������� ������
			ifstream file(tokens[1], ios::binary);
			if (!file.is_open())
			{
				cout << "���� �� ����� ���� ������!\n" << endl;
				continue;
			}
			else
			{
				// ��������� ������� �����
				int fileSize = file_size(tokens[1]);
				// ��������� ������ ��� �������� ������ �����
				char* data = new char[fileSize];

				// ������ ������ �����
				file.read(data, fileSize);

				// ������������ ������� mPUT � ������ �����, �������� � �������
				command = "mPUT|" + tokens[1] + "|" + to_string(fileSize) + "|";
				command.append(data, fileSize);

				// ������������ ������ � �������� �����
				delete[] data;
				file.close();
			}
		}

		// �������� ������� �������
		if (send(clientSocket, command.c_str(), command.length(), 0) == SOCKET_ERROR)
		{
			cerr << "������ ��� �������� ������ �������" << endl;
			break;
		}

		// ��������� ������ �� �������
		char buffer[MAX_BUFFER_SIZE];
		int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesRead == SOCKET_ERROR || bytesRead == 0)
		{
			cerr << "������ ��� ��������� ������ �� �������" << endl;
			break;
		}

		// ����� ���������� �� �����
		string result(buffer, bytesRead);
		cout << "���������: " << result << endl;

		// �������� �� ������� bye ��� ���������� ������
		if (command == "bye")
		{
			break;
		}
	}

	// �������� ����������� ������
	closesocket(clientSocket);
	// ������� Winsock
	WSACleanup();

	// ���������� ���������
	return 0;
}
