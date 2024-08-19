#include <iostream>         // ����������� ���������� �����-������
#include <winsock2.h>       // ������������ ���� ��� ������ � �������� � windows
#include <ws2tcpip.h>       // ������������ ���� � ��������������� ��������� �� ������ � �������� � windows
#include <vector>           // ���������� ��� �������� ���������� ������� � ������ � ��������
#include <string>           // ���������� ��� ������ �� ��������
#include <mutex>            // ���������� ��� ������������� ������� � ����� �������� � ������������� �����
#include <ctime>            // ���������� ��� ����������� ������� (�������� �������� �� �������)
#include <thread>           // ���������� ��� ������������� ��������������� (jthread ��������� � ��������� ISO C++ 20)
#include <filesystem>       // ���������� ��� ������� ������ � �������� �������� (�������� ISO C++ 17+)
#include <syncstream>       // ���������� ��� ������������� ����������� ���������� ������ (�������� ISO C++ 20)


// ����� ��������� ������������� ���������� ���������� WinSock 
#pragma comment(lib, "ws2_32.lib")
// ����� ��������� ������������� ��������� ������, ��������� � �������������� ���������� ������� (C4996)
#pragma warning(disable : 4996)


// ��������� ������������ ���� std � filesystem, ����� �� ������ ��������������� �������� std:: � filesystem::
using namespace std;
using namespace filesystem;


// ����� �����
const unsigned short int ServerPort = htons(83);
// ����������� ���������� �������� � ������� (����� ������������ ��������� SOMAXCONN � ������� �������� ������������ ����� ��������� ����������)
const unsigned short int maxCount = 2;
// ������� ������������ ��������
unsigned int clientCounter = 0;
// ���� ���������� ��������� (����� ��� ��������� ������������ ��������, � ����� ��� ��������� ����������� ��������)
bool flagReady = 0;
// ���������� ��� ������������� ������� (����� lock_guard ������� �� ���������, �� ����� ������� � ��������, �.�. unlock ���������� � �����������)
mutex mutexThreadList, mutexSend;
// ��������� ����� (����������, ������ ��� ������������ � ���� ��������)
SOCKET listeningSocket;
// ������ ��� ������ ��������
vector<SOCKET> socketList;
// ������ ��� ������ ��� �������� (jthread - join thread, ����� ���������, ������� ����� ���� ������������� � ��������� ������ ��� ������ �����������)
vector<jthread> threadList;


// ������������� Windows Sockets DLL
int WinSockInit()
{
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;

    // ������� ��������� � ������ DLL
    cout << "SERVER: Starting WinSock..." << endl;

    // ����������������� WinSock, ���� �������� ������, �� ��������� DLL � ������� �� ������� � ����� 1
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
        cout << "SERVER: Error: Couldn't find a usable WinSock DLL." << endl;
        WSACleanup();
        return 1;
    }

    // �������� ������ WinSock, ���� ������ �� 2.2, �� ��������� DLL � ������� �� ������� � ����� 1
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        cout << "SERVER: Error: Couldn't find a usable WinSock DLL." << endl;
        WSACleanup();
        return 1;
    }

    // ���� ������������� � �������� ������ �� ������� ������, �� ������� ��������� � ���������� �������� 0
    cout << "SERVER: WinSock started successfully." << endl;
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
            cout << "\rSERVER: Data transmission stopped." << endl;
            // ������� ��������� � �������� ������
            cout << "\rSERVER: Listening socket [" << socket << "] successfully closed." << endl;
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
            cout << "\rSERVER: Data transmission stopped." << endl;
            // ������� ��������� � �������� ������
            cout << "\rSERVER: Listening socket [" << socket << "] successfully closed." << endl;
            // ������� ��������� � �������� �������
            cout << "\rSERVER: WinSock was successfully closed." << endl;
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

// ������� �������
void serverCommands(SOCKET listeningSocket)
{
    // �������������� ��������� � �� ������ ��� ��������� � ������ ������ � ��������
    sockaddr_in addr;
    int clientAddrSize = sizeof(addr);

    // ������� ���������� ��� ������ � ������������ �������
    char word[8];
    const unsigned short int bufferSize = 8;
    while (1)
    {
        // ������� ������ �������
        while (!flagReady) {};

        // ������� ������-�����������
        osyncstream(cout) << "\033[32m>\033[0m ";
        // � ������� ���� � ����������
        cin.getline(word, bufferSize);

        // ���� ���� �������� ������� help
        if (strcmp(word, "help") == 0)
        {
            osyncstream(cout) << "\rINFO: help - shows server commands" << endl;
            osyncstream(cout) << "\rINFO: who - shows current connections" << endl;
            osyncstream(cout) << "\rINFO: stop - disconnects all clients, stops work in quiet mode" << endl;
        }

        // ���� ���� �������� ������� who
        else if (strcmp(word, "who") == 0)
        {
            // ���������� ��-�������� ���� �������� � ������� ���������� � ���
            for (int i = 0; i < socketList.size(); i++) {
                getpeername(socketList.at(i), (sockaddr*)&addr, &clientAddrSize);
                osyncstream(cout) << "\rINFO: Client [" << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port) <<
                    "] with socket [" << socketList.at(i) << "] is served by thread [" << threadList.at(i).get_id() << ']' << endl;
            }

            // ���� ������ ������� ���� � ������ �� ������� ������ ���
            if (socketList.size() == 0) {
                osyncstream(cout) << "\rINFO: there is no one on the server right now, but it doesn't matter, someone will definitely join" << endl;
            }
        }

        // ���� ���� �������� ������� stop
        else if (strcmp(word, "stop") == 0)
        {
            // ������� ���� ���������� ������ ���������
            flagReady = 0;

            // ����������� ��� ������ �� �������� ���������
            for (auto& thread : threadList)
            {
                thread.detach();
            }

            // ���������� ������ ���� ��������
            for (auto& curSocket : socketList)
            {
                // �������� ������ � �������� ����� �� �����
                getpeername(curSocket, (sockaddr*)&addr, &clientAddrSize);
                // ������� ���������
                osyncstream(cout) << "\rINFO: Client [" << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port)
                    << "] is disconnected from the server." << endl;
                // ��������� ������ � ������� �������
                WinSockOff(curSocket, 1, 0, NULL);
            }

            // ������� ������� ������� � �������
            threadList.clear();
            socketList.clear();

            // ��������� ��� �������� ������ �������
            WinSockOff(listeningSocket, 0, 1, NULL);

            // �������� ������ ������
            return;
        }
        else
        {
            // ���� ����� ���� ������������ �������, ������ ��� �� ����
            osyncstream(cout) << "\rUnknown command, write 'help' to see the list of available commands." << endl;
        }
    }
}

// ������ � ���������
void doExchange(SOCKET s, sockaddr_in addr)
{
    // �������� ID ������� ������
    jthread::id currentThreadId = this_thread::get_id();

    // ������� 2 ���������� ������� ��� ���������� ��������� ������ � ������ �������������
    char port_str[6];       // ������������ ����� ����� - 5 �������� + 1 ������ ��� ������������ ����
    char socket_str[17];    // ������������ ����� ������ ��� IPv4 - 16 �������� + 1 ������ ��� ������������ ����

    // ������� ���������� ��� ������ � ������������ �������
    char* word;
    const unsigned short int bufferSize = 1024;
    unsigned short int clientID = 0;
    string sendMessage;

    while (1)
    {
        // ����������� �������� ������ ��� ����� ���������
        word = new char[bufferSize];

        // ���� ������ ������ 0 ����, �� �������� � ����������� �����������
        if (recv(s, word, bufferSize, 0) > 0)
        {
            // ���� ������ ������� bye
            if (strcmp(word, "bye") == 0) break;

            // ���� ������ ������� who
            else if (strcmp(word, "help") == 0)
            {
                // ��������� ������ ������ ������� � �������� ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
                lock_guard<mutex> guard(mutexSend);

                // ��������� ��������� ��� ��������
                sendMessage = "help - shows a list of available commands\n"
                    "bye - to break the connection with the server\n"
                    "who - shows current connections\n"
                    "ping - receive 20 packets from the server indicating the response rate in milliseconds\n"
                    "cd 'path' - changing the directory on the server\n"
                    "pwd - shows the current directory on the server\n"
                    "ls - shows a list of files in the current server directory";

                // ���������� ��������� �������
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            // ���� ������ ������� who
            else if (strcmp(word, "who") == 0)
            {
                // ��������� ������ ������ ������� � �������� ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
                lock_guard<mutex> guard(mutexSend);

                // �������������� ��������� � �� ������ ��� ��������� � ������ ������ � ��������
                sockaddr_in addr;
                int clientAddrSize = sizeof(addr);
                // �������������� ���������� ��� �����������
                sendMessage = '\r';

                for (const auto& socked : socketList)
                {
                    // �������� ������ � �������� ����� �� �����
                    getpeername(socked, (sockaddr*)&addr, &clientAddrSize);
                    snprintf(port_str, sizeof(port_str), "%i", ntohs(addr.sin_port));
                    snprintf(socket_str, sizeof(socket_str), "%llu", socked);

                    // ��������� ��������� ��� ��������
                    sendMessage += "Client [";
                    sendMessage += inet_ntoa(addr.sin_addr);
                    sendMessage += ':';
                    sendMessage += port_str;
                    sendMessage += "] with socket [";
                    sendMessage += socket_str;
                    sendMessage += "] is online.\n";
                }
                // ��������� ������� ��������� ������
                sendMessage += "END_OF_LIST_USERS";
                // ���������� ��������� �������
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            // ���� ������ ������� pwd
            else if (strcmp(word, "pwd") == 0)
            {
                // ��������� ������ ������ ������� � �������� ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
                lock_guard<mutex> guard(mutexSend);

                // �������� ���� ������� ���������� � ���� string
                sendMessage = current_path().string();

                // ���������� ������� ���� ������� ����������
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            // ���� ������ ������� cd...
            else if (strstr(word, "cd"))
            {
                // ��������� ������ ������ ������� � �������� ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
                lock_guard<mutex> guard(mutexSend);

                // ����� ����������� ������, � �������� ��� ����� ������� � ����������� ���������� �������� ��� �����
                try
                {
                    // ��������� �� ������� �������� - ����������
                    string directory = string(word + 3);

                    // ��������� � �������� ����������
                    current_path(directory);

                    // �������������� ����� � �������� �������
                    sendMessage = "Directory has been successfully changed to ";
                    sendMessage += directory;

                    // ���������� ������� ��������� �� �������� ��������
                    send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
                }
                catch (const exception& e)
                {
                    // ����� ��������� ����� ����� ������ � ������ �� ����� ����� ����������
                    cerr << "\rSERVER: Unexpected error with the code:" << e.what() << endl;
                    // � ������ ������ ���������� �������, ��� ������� ���������� �� �������
                    send(s, "Unable to change directory", sizeof("Unable to change directory"), 0);
                }
            }


            // ���� ������ ������� ls
            else if (strcmp(word, "ls") == 0)
            {
                // ��������� ������ ������ ������� � �������� ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
                lock_guard<mutex> guard(mutexSend);

                // �������������� ��������, ������� ����� ��������� �� ����� ������� ����������
                directory_iterator cursor(".");

                // �������������� ����������, � ������� ����� ��������� ������ ������ (������� �����, ����� ��������� ����� Recived)
                sendMessage = '\r';

                // ���������� ��� ��������, �� ������� ������ ��������� �������� (��� ����� ������� ����������)
                for (const auto& entry : cursor)
                {
                    // ��������� � ���������� ���� � �������� ����� � ���� ������
                    string filename = entry.path().filename().string();
                    sendMessage += filename;
                    sendMessage += '\n';
                }
                // ��������� ������� ��������� ������
                sendMessage += "END_OF_LIST_FILES";

                // ��������� ������� ������ ������ � ������� ���������� � ���� ������ � ������� ������� ���� � ��� �����
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            else if (strcmp(word, "ping") == 0)
            {
                // ��������� ������ ������ ������� � �������� ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
                lock_guard<mutex> guard(mutexSend);

                // ������������� ����� ��� ������� ������� � �������� �������� ���������� �������
                srand(static_cast<unsigned int>(time(NULL)));

                // ���������� 20 �������
                for (int i = 0; i < 20; i++)
                {   // ��������� �������� � ���������� ��������� ����� ��� ������ �������� (���������� ����� �� 100�� �� 199��)
                    Sleep((rand() % 100) + 100);
                    send(s, "0x762f62ff", sizeof("0x762f62ff"), 0);
                }

                // ������������� �������� ��� �������� �������� ������, ������� ����������� �������� ��������
                Sleep(100);
                send(s, "stop", sizeof("stop"), 0);

                // ���������� �������� � 100��, ����� �������� ������ ������ ��� �������� ��������� � �������� ���������� �������� ��������
                Sleep(100);
                send(s, "\rINFO: Pheck completed", sizeof("\rINFO: Pheck completed"), 0);
            }

            else
            {
                // ��������� ������ ������ ������� � �������� ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
                lock_guard<mutex> guard(mutexSend);

                // �������������� ����� � �������� �������
                string sendMessage = "Unknown command, write 'help' to see the list of available commands.";

                // ���������� ������� ��������� � ������������ �������
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            // ���� ������ �� �������, �� ���������� �� ������ ����� � ���� �����������, � ����� ����� ���������
            // ���������� ����� osyncstream ��� ���������� ����������������� ������ � �������
            osyncstream(cout) << "\rINFO: Received from " << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port) << " - " << word << endl;
            osyncstream(cout) << "\033[32m>\033[0m ";
        }
        // ���� ������ ���������� ��� ������ ����� ���������� ��������� �� ������ ���������� � ���
        else break;

        // ������� ����� ��� �������� ��������� (��� �������, ��� ������� ����������)
        delete[] word;

        // ������ �� ����� �������� ���� �� ���� ������� bye � ������ > 0 ���� �� �������
    }

    // ���� ������ ��������� � ������� ���������, �� �������� ����� � ��������, �� ��������� ���
    if (flagReady)
    {
        // ��������� ������ ������ ������� ������ � ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
        {
            lock_guard<mutex> guard(mutexThreadList);

            // ���������� ��� �������� ������ ���� �� ������ ������� ��������
            for (int i = 0; i < threadList.size(); i++) {
                if (currentThreadId == threadList.at(i).get_id()) {
                    clientID = i;
                    break;
                }
            }

            // ������� ����� ������� ������� �� ������� �������
            socketList.erase(socketList.begin() + clientID);
            // ��������� ������� �������� ��������
            clientCounter--;
            // ����������� ����� �� �������� ���������
            threadList.at(clientID).detach();
            // ������� ����� �� ������� �������, ��� ����� �������� ���
            threadList.erase(threadList.begin() + clientID);
        }

        // ��������� ������ � ������������ �������
        WinSockOff(s, 1, 0, NULL);

        // ������� ��������
        delete[] word;

        // ����� ����������
        osyncstream(cout) << "\rSERVER: User [" << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port) << "] disconect from server" << endl;
        osyncstream(cout) << "\033[32m>\033[0m ";
    }
}

// �������� �������
int main()
{
    // ������� ���������� ��� ����� ��� ������ � ���������
    SOCKET clientSocket;

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
        cout << "\rSERVER: Error creating listening socket. Error code: " << WSAGetLastError() << endl;
        // ��������� WinSock
        WSACleanup();
        // ������� ��������� � �������� �������
        cout << "\rSERVER: WinSock was successfully closed." << endl;
        // ������� � �������
        return -2;
    }
    // ����� � "���������" ������ ����������� ��� ����������, ������� ��� �� �����
    else cout << "\rSERVER: Listening socket [" << listeningSocket << "] successfully created." << endl;



    // ������� ����� ��� ��������� ������
    jthread commands(serverCommands, listeningSocket);

    // ������ ��������� ��� ������� �����������
    hostAddres.sin_addr.s_addr = INADDR_ANY;  // ����� IP-�����
    hostAddres.sin_family = AF_INET;          // IPv4
    hostAddres.sin_port = ServerPort;         // ����

    // ��������� ��������� �� ���������� �������, ���� �������� �� ������� ������� � ������� -2
    if (bind(listeningSocket, (sockaddr*)&hostAddres, sizeof(hostAddres)))
    {
        // ������� ���������� �� ������ �� �����
        cout << "\rSERVER: Error binding to hostAddres [" << inet_ntoa(hostAddres.sin_addr) << ':' << ntohs(ServerPort) << "] Error code : " << WSAGetLastError() << endl;
        // ��������� ������ �� ��������� �������
        WinSockOff(listeningSocket, 0, 1, -3);
    }
    // ����� ��������� ���� ������� ��������� � ������, ������� ��������� �� �����
    else
    {
        // ������� ������ �� "����������" ������ �� �����
        cout << "\rSERVER: HostAddres [" << inet_ntoa(hostAddres.sin_addr) << ':' << ntohs(ServerPort) <<
            "] successfully bound to socket [" << listeningSocket << ']' << endl;
    }



    // ������������� "���������" ����� � ����� "�������������" ��� ������������� ���������� ����������� ��������� � �������
    if (listen(listeningSocket, maxCount))
    {
        // ������� ���������� �� ������ �� �����
        cout << "\rSERVER: Error listening on socket. Error code: " << WSAGetLastError() << endl;
        // ��������� ������ �� ��������� �������
        WinSockOff(listeningSocket, 0, 1, -4);
    }
    // ����� "���������" ������� ��������, ������� ��������� �� �����
    else
    {
        // ������� ������ �� "����������" ������ �� �����
        cout << "\rSERVER: Socket [" << listeningSocket << "] has started listening for incoming connections." << endl <<
            "SERVER: Maximum number of clients on the server [" << maxCount << ']' << endl;
    }



    // ������� ��������� �� �������� �������� � ��������� � ������������ �������
    cout << "\rSERVER: Waiting for clients to connect..." << endl <<
        "SERVER: Type 'help' to view the available commands." << endl;

    // ����������, ��� ������ ����� � ������
    flagReady = 1;

    // ������� ����� ��� ������������� ����������� �������� ��������
waiting:
    // ����������� ���� �� ��������� �����������
    while (1)
    {
        // ����� �������������� ����������� ���������� ��� ����������� ����� ��������
        try
        {
            // ��������� ����������, ���� ������� ������� INVALID_SOCKET, �� ������� ��������� �� ������ � ���������� ������� �������
            if (((clientSocket = accept(listeningSocket, (sockaddr*)&hostAddres, &addrSize)) == INVALID_SOCKET))
            {
                // ���� ������ ��������, �� ����� �� ����� � ���������� ���������
                if (!flagReady) break;
                // ���������� ����� osyncstream ��� ���������� ����������������� ������ � �������
                osyncstream(cout) << "\rSERVER: Error accepting connection. Error code: " << WSAGetLastError() << endl;
                continue;
            }
            else
            {
                // ���� ������ �������� ��������� ���� ����� ��������
                if (clientCounter >= maxCount)
                {
                    // ������� ��������� � ���, ��� � ��� ������ ������������, �� ��������� ���� ���
                    // ���������� ����� osyncstream ��� ���������� ����������������� ������ � �������
                    osyncstream(cout) << "\rSERVER: Client [" << inet_ntoa(hostAddres.sin_addr) << ':'
                        << ntohs(hostAddres.sin_port) << "] and socket [" << clientSocket << "] tried to connect, but "
                        << "the server is full" << endl << flush << "\033[32m>\033[0m " << flush;

                    WinSockOff(clientSocket, 1, 0, NULL);
                    continue;
                }

                // ���� ����������� ������ �������, �� ������� �� ����� ������ � ������������
                // ���������� ����� osyncstream ��� ���������� ����������������� ������ � �������
                osyncstream(cout) << "\rSERVER: Connection accepted from [" << inet_ntoa(hostAddres.sin_addr) << ':'
                    << ntohs(hostAddres.sin_port) << "] and socket [" << clientSocket << ']' << endl << flush;
                osyncstream(cout) << "\033[32m>\033[0m " << flush;

                {
                    // ��������� ������ ������ ������� ������ � ��������� ����� lock_guard (� ������������ �������� lock, � � ����������� unlock)
                    lock_guard<mutex> guard(mutexThreadList);

                    // ��������� ����� � ������ (push_back �.�. ��������� ������������ ������ � ������ �������)
                    socketList.push_back(clientSocket);
                    // ������� ����� ����� ��� ������������ ����� ������� (emplace_back �.�. ������� ����� ������ � ������ �������)
                    threadList.emplace_back(jthread(doExchange, clientSocket, hostAddres));
                }
                // ����������� ������� �����������
                clientCounter++;
            }
        }
        // ���� ��������� �����-�� ���������� ����� �� � ���������� ������� ������� 
        catch (const system_error& e)
        {
            // ����� ��������� ����� ����� ������ � �������������� ������
            cerr << "\rSERVER: Unexpected error with the code:" << e.what() << endl;
            continue;
        }
    }
    // ��� ���������� �������� �� ������ �����������, ������� ����� �� ��� ������



    // ���� ������ �������� ����� �� ������������ ������������ ��������
    while (flagReady)
    {
        // ���� �� ������� ���������� �����, �� ��������� �� ����������� ����� ��������
        if (clientCounter < maxCount) goto waiting;
    }



    // ������� ��������� � �������� �������
    cout << "\rSERVER: The server has been successfully stopped." << endl;

    // ����������� ����� ��� ��������� ������
    commands.detach();

    // ���� ������ ����� �� ������ �����, ������ ��� ���������� �������� stop � ��� �������� �� ���������� ��������, �������� ������� � �.�. ���������
    return 0;
}
