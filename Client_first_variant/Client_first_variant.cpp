#include <iostream>         // ����������� ���������� �����-������
#include <winsock2.h>       // ������������ ���� ��� ������ � �������� � windows
#include <ws2tcpip.h>       // ������������ ���� � ��������������� ��������� �� ������ � �������� � windows
#include <vector>           // ���������� ��� �������� ���������� ������� � ������ � ��������
#include <string>           // ���������� ��� ������ �� ��������
#include <thread>           // ���������� ��� ������������� ��������������� (jthread ��������� � ��������� ISO C++ 20)
#include <algorithm>	    // ���������� ��� ������ � ������������ STL (��� ����� ������ ����� transform())
#include <cctype>	    // ���������� ��� ������ � ��������� (��� ����� ������ ����� tolower())


// ����� ��������� ������������� ���������� ���������� WinSock 
#pragma comment(lib, "ws2_32.lib")
// ����� ��������� ������������� ��������� ������, ��������� � �������������� ���������� ������� (C4996)
#pragma warning(disable : 4996)


// ��������� ������������ ���� std, ����� �� ������ ��������������� ������� std::
using namespace std;
double start_time = 0, end_time = 0;	// ���������� ��� ������ �������
bool flag_ping = 0;						// ���� ��� ����������� ������� �����

//==================================================================================================//
const unsigned int ServerIP = inet_addr("172.20.10.4");		// IPv4 ����� ��������� ���� �������	//
const unsigned short int ServerPort = htons(83);			// ����� �����							//
//==================================================================================================//


// ������������� Windows Sockets DLL
int WinSockInit()
{
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;

	// ������� ��������� � ������ DLL
	cout << "INFO: Starting WinSock..." << endl;

	// ����������������� WinSock, ���� �������� ������, �� ��������� DLL � ������� �� ������� � ����� 1
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		cout << "INFO: Error: Couldn't find a usable WinSock DLL." << endl;
		WSACleanup();
		return 1;
	}

	// �������� ������ WinSock, ���� ������ �� 2.2, �� ��������� DLL � ������� �� ������� � ����� 1
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		cout << "INFO: Error: Couldn't find a usable WinSock DLL." << endl;
		WSACleanup();
		return 1;
	}

	// ���� ������������� � �������� ������ �� ������� ������, �� ������� ��������� � ���������� �������� 0
	cout << "INFO: WinSock started successfully." << endl;
	return 0;
}


// ���������� Windows Sockets DLL
// socket (���������� ������ ��� ����������)
// onlyTCP (1 - ������ ��� ���������� �������� ������, 0 - ��� ������ ��������� ���� ��������� DLL)
// needMsg (1 - �������� ����������� � ������ �������, 0 - ��������� ���������� � "�����" ������)
// errCode (NULL - ���� ���������� �� �����, ����� ��� � ������� ����� ��������� ���������)
void WinSockOff(SOCKET socket, bool onlyTCP, bool needMsg, int errCode)
{
	if (onlyTCP)
	{
		if (needMsg)
		{
			// ������� ��������� �� ��������� �������� ������
			cout << "\rINFO: Data transmission stopped." << endl;
			// ������� ��������� � �������� ������
			cout << "\rINFO: Listening socket [" << socket << "] successfully closed." << endl;
		}

		// ��������� �������� ������ ��� ���������� ������
		shutdown(socket, SD_SEND);
		// �������� ���������� ������
		closesocket(socket);
	}
	else
	{
		if (needMsg)
		{
			// ������� ��������� �� ��������� �������� ������
			cout << "\rINFO: Data transmission stopped." << endl;
			// ������� ��������� � �������� ������
			cout << "\rINFO: Listening socket [" << socket << "] successfully closed." << endl;
			// ������� ��������� � �������� �������
			cout << "\rINFO: WinSock was successfully closed." << endl;
		}

		// ��������� �������� ������ ��� ���������� ������
		shutdown(socket, SD_SEND);
		// �������� ���������� ������
		closesocket(socket);
		// ��������� WinSock
		WSACleanup();
	}

	if (errCode != NULL) exit(errCode);
}


// �������� ������
void doSend(SOCKET s)
{
	// �������������� ������� ����� � ������ ��� �������� ������ ��������� �������������
	char word[1024];
	string str;

	// ����������� ����
	while (1)
	{
		// ������� ������-�����������
		cout << "\033[32m>\033[0m ";
		// ����������� � ������������ ���� ������, ��������� �������� � ����� word. ������������ ����� 1024 �������.
		cin.getline(word, 1024);
		// �������� ������ �������� � ������ ��� ������� ������
		str = word;

		// ��������� ������� ���� ������� �������� � �������������� �� � �������� ����� ��� ������������ ��������� �������
		if (str.length() == 2) transform(str.begin(), str.begin() + 2, str.begin(), [](unsigned char c) { return tolower(c); });
		if (str.length() == 3) transform(str.begin(), str.begin() + 3, str.begin(), [](unsigned char c) { return tolower(c); });
		if (str.length() == 4) transform(str.begin(), str.begin() + 4, str.begin(), [](unsigned char c) { return tolower(c); });

		// ���� ������ ������� bye, ������ �� ����� ����������� �� �������. ������� �� ����� � ������ ���
		if (strcmp(str.c_str(), "bye") == 0) break;

		// ���� ������ ������� ping, ������ ����� �������� ���� �� �������, �������� ��� ���������� � ����� �������� �������
		if (strcmp(str.c_str(), "ping") == 0)
		{
			// ����� ��������������� ���������
			cout << "\rINFO: You will be sent 20 packets to measure the connection speed" << endl;

			// �������� ����� �������� ����� ������� �����
			flag_ping = 1;
			// �������� �����
			double ticks = clock();
			start_time = ticks / CLOCKS_PER_SEC * 1000.0;
		}

		// ���������� ���������� �� ������������ ������ �� ������ word, ������� ��������� �������� +1 
		// (����� ������ ������ �������� ��������� ������ �.�. strlen �� ��������� '\0')
		send(s, str.c_str(), static_cast<int>(str.length()) + 1, 0);

		// ������ �� ����� �������� ���� �� ���� ������� bye
	}

	// ���������� ������� ���������� ����� �� ������ word, ������� +1 
	// (����� ������ ������ �������� ��������� ������ �.�. strlen �� ��������� '\0')
	send(s, str.c_str(), static_cast<int>(str.length()) + 1, 0);

	// ����� �� �������
	return;
}
// ����� ������
void doReceiv(SOCKET s)
{
	// ������� ���������� ��� ������ � ������������ �������
	char* word;
	const unsigned short int bufferSize = 1024;

	while (1)
	{
		// ����������� �������� ������ ��� ����� ���������
		word = new char[bufferSize];

		// ���� ������ ������ 0 ����, �� �������� � ����������� �����������
		if (recv(s, word, bufferSize, 0) > 0)
		{
			// ���� �� ��������� ����� �������, �� ��� ��������� ����� stop ������������ ��� ����� ��� ������
			if (flag_ping)
			{
				// ���� ������ ������� stop
				if (strcmp(word, "stop") == 0)
				{
					// ������� �� ������ ��������� ������� �����
					flag_ping = 0;
					// ��������� � ������ ������ ���������
					continue;
				}
				// ���� ������ �����
				else
				{
					// �������� ����� �������� ���������� ������ �����
					double ticks = clock();
					end_time = ticks / CLOCKS_PER_SEC * 1000.0;

					// ����������� ������� �� ������� ����� �������� �����
					double result = end_time - start_time;
					// ���� ����� ������ 2.5 ������
					if (result > 2500)
					{
						// ������� ��������� � ���, ��� ����� �������� ���������
						cout << "\rINFO: Waiting time exceeded: " << endl;
						// ������� �� ������ ��������� ������� ����� 
						flag_ping = 0;
						// ��������� � ������ ������ ���������
						continue;
					}
					// ���� ����� ����� �������� ������ 2.5 ������
					else
					{
						// ������� ������� ������ �������� � ��������� ������ �����
						cout << "\rping: " << result << endl;

						// �������� ����� �� ��������� ������ ������ �����
						double ticks = clock();
						start_time = ticks / CLOCKS_PER_SEC * 1000.0;
						// ��������� � ������ ������ ������
						continue;
					}
				}
			}

			// ��������� �� ������� �� ����� ������ ������� bye, ������ ������� ��������� ����� ��������
			cout << '\r' << word << endl;
			// ������� ������-�����������
			cout << "\033[32m>\033[0m ";
		}
		// ���� ����� � �������� �������� ��� ������ ����� ���������� ��������� �� ������ ���������� � ���
		else break;

		// ������� ������ ����� ��������� ������� � ����
		delete[] word;
	}

	// ������� ������ ����� ������� �� ������� (���� ���� ������� ������� break, �� delete �� 174 �� ����������)
	delete[] word;

	// ����� �� �������
	return;
}


// �������� �������
int main()
{
	// ������� ���������� ��� �����
	SOCKET listeningSocket;

	// ��������� ��� �������� ������� ������
	struct sockaddr_in hostAddres;

	// �������������� ���������� ��� ������ ���������, � ������� ����� ���������� ���������� � ������ ������
	int addrSize = sizeof(sockaddr_in);



	// �������������� WinSock, ���� ������������� �� �������, �� ������� � ����� -1
	if (WinSockInit()) return -1;



	// �������������� "���������" ����� ��� ������ ��������, ���� ������� ������� INVALID_SOCKET, �� ������� � ������� -2
	if ((listeningSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		// ������� ��������� �� ������
		cout << "\rINFO: Error creating listening socket. Error code: " << WSAGetLastError() << endl;
		// ��������� WinSock
		WSACleanup();
		// ������� ��������� � �������� �������
		cout << "\rINFO: WinSock was successfully closed." << endl;
		// ������� � �������
		return -2;
	}
	// ����� � "���������" ������ ����������� ��� ����������, ������� ��� �� �����
	else cout << "\rINFO: Listening socket [" << listeningSocket << "] successfully created." << endl;



	// ��������� ���������� ������ (IPv4, ����, IP-�����)
	hostAddres.sin_family = AF_INET;
	hostAddres.sin_port = ServerPort;
	hostAddres.sin_addr.s_addr = ServerIP;

	// ��������� ����������� � ��������� �����������, ���� �� ���������� ��
	if (connect(listeningSocket, (sockaddr*)&hostAddres, sizeof(hostAddres)))
	{
		// ������� ��������� � ���, ��� ������ ��� ����������� �� ������
		cout << "\rINFO: Server not found. Error code: " << WSAGetLastError() << endl;
		// ��������� ������ �� ��������� �������
		WinSockOff(listeningSocket, 0, 1, -3);
	}



	// ������� ��� ������ �� �������� � ��������� ������
	thread rec(doReceiv, listeningSocket);
	thread snd(doSend, listeningSocket);



	// ������� �������
	system("cls");
	// ������� ��������� � �������� ����������� � ��������� 'help'
	cout << "\rINFO: The connection is successful, enter 'help' to view the available commands." << endl;
	// ������� ������-�����������
	cout << "\033[32m>\033[0m ";

	// ���� ���� ����� ������ �� �������� ������ (���� �� ��������� ���������� � ��������)
	rec.join();
	snd.detach();



	// ����� ������ ��������� ���� ������ ������� ���������, ��� ����� ��������
	cout << "\rINFO: You have been disconnected from the server." << endl;

	// ��������� ������ �� ��������� �������
	WinSockOff(listeningSocket, 0, 1, -4);

	// ������� ������������
	return 0;
}
