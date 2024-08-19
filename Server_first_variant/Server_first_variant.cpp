#include <iostream>         // стандартная библиотека ввода-вывода
#include <winsock2.h>       // заголовочный файл для работы с сокетами в windows
#include <ws2tcpip.h>       // заголовочный файл с дополнительными функциями по работе с сокетами в windows
#include <vector>           // библиотека для удобного размещения потоков и данных о клиентах
#include <string>           // библиотека для работы со строками
#include <mutex>            // библиотека для синхронизации доступа к общим ресурсам в многопоточной среде
#include <ctime>            // библиотека для подключения рандома (имитация задержки на сервере)
#include <thread>           // библиотека для осуществления многопоточности (jthread появились в стандарте ISO C++ 20)
#include <filesystem>       // библиотека для удобной работы с файловой системой (стандарт ISO C++ 17+)
#include <syncstream>       // библиотека для осуществления безопасного потокового вывода (стандарт ISO C++ 20)


// через директиву препроцессора подключаем библиотеку WinSock 
#pragma comment(lib, "ws2_32.lib")
// через директиву препроцессора отключаем ошибки, связанные с использованием устаревших функций (C4996)
#pragma warning(disable : 4996)


// объявляем пространство имен std и filesystem, чтобы не писать соответствующие префиксы std:: и filesystem::
using namespace std;
using namespace filesystem;


// номер порта
const unsigned short int ServerPort = htons(83);
// максимально количество клиентов в очереди (можно использовать константу SOMAXCONN в которой хранится максимальное число ожидающих соединений)
const unsigned short int maxCount = 2;
// счетчик подключенных клиентов
unsigned int clientCounter = 0;
// флаг готовности программы (нужен для некоторых декоративных моментов, а также для обработки подключений клиентов)
bool flagReady = 0;
// переменная для синхронизации потоков (класс lock_guard основан на мьютексах, но более удобный в закрытии, т.к. unlock вызывается в деструкторе)
mutex mutexThreadList, mutexSend;
// слушающий сокет (глобальный, потому что используется в двух функциях)
SOCKET listeningSocket;
// вектор под сокеты клиентов
vector<SOCKET> socketList;
// вектор под потоки для клиентов (jthread - join thread, новая приколюха, которая может сама присоединятся к основному потоку при вызове дескриптора)
vector<jthread> threadList;


// Инициализация Windows Sockets DLL
int WinSockInit()
{
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;

    // выводим сообщение о старте DLL
    cout << "SERVER: Starting WinSock..." << endl;

    // Проинициализируем WinSock, если возникли ошибки, то отключаем DLL и выходим из функции с кодом 1
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
        cout << "SERVER: Error: Couldn't find a usable WinSock DLL." << endl;
        WSACleanup();
        return 1;
    }

    // Проверка версии WinSock, если версия не 2.2, то отключаем DLL и выходим из функции с кодом 1
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        cout << "SERVER: Error: Couldn't find a usable WinSock DLL." << endl;
        WSACleanup();
        return 1;
    }

    // Если инициализация и проверка версии не вызвали ошибок, то выводим сообщение и возвращаем значение 0
    cout << "SERVER: WinSock started successfully." << endl;
    return 0;
}


// Отключение Windows Sockets DLL
// socket (дескриптор сокета для выключения)
// onlyTCP (1 - только для отключения передачи данных, 0 - для полной остановки всех процессов DLL)
// needMsg (1 - включить комментарии о работе функций, 0 - выполнить отключение в "тихом" режиме)
// errCode (NULL - если завершение не нужно, любой код с которым нужно завершить программу)
void WinSockOff(SOCKET socket, bool onlyTCP, bool needMsg, int errCode)
{
    if (onlyTCP)
    {
        if (needMsg)
        {
            // выводим сообщение об остановке передачи данных
            cout << "\rSERVER: Data transmission stopped." << endl;
            // выводим сообщение о закрытии сокета
            cout << "\rSERVER: Listening socket [" << socket << "] successfully closed." << endl;
        }

        // остановка передачи данных для слушающего сокета
        shutdown(socket, SD_SEND);
        // закрытие слушающего сокета
        closesocket(socket);
    }
    else
    {
        if (needMsg)
        {
            // выводим сообщение об остановке передачи данных
            cout << "\rSERVER: Data transmission stopped." << endl;
            // выводим сообщение о закрытии сокета
            cout << "\rSERVER: Listening socket [" << socket << "] successfully closed." << endl;
            // выводим сообщение о закрытии сервера
            cout << "\rSERVER: WinSock was successfully closed." << endl;
        }

        // остановка передачи данных для слушающего сокета
        shutdown(socket, SD_SEND);
        // закрытие слушающего сокета
        closesocket(socket);
        // отключаем WinSock
        WSACleanup();
    }

    if (errCode != NULL) exit(errCode);
}

// команды сервера
void serverCommands(SOCKET listeningSocket)
{
    // инициализируем структуру и ее размер для получения и вывода данных о клиентах
    sockaddr_in addr;
    int clientAddrSize = sizeof(addr);

    // создаем переменные для работы с поступившими данными
    char word[8];
    const unsigned short int bufferSize = 8;
    while (1)
    {
        // ожидаем запуск сервера
        while (!flagReady) {};

        // выводим символ-приглашение
        osyncstream(cout) << "\033[32m>\033[0m ";
        // и ожидаем ввод с клавиатуры
        cin.getline(word, bufferSize);

        // если была написана команда help
        if (strcmp(word, "help") == 0)
        {
            osyncstream(cout) << "\rINFO: help - shows server commands" << endl;
            osyncstream(cout) << "\rINFO: who - shows current connections" << endl;
            osyncstream(cout) << "\rINFO: stop - disconnects all clients, stops work in quiet mode" << endl;
        }

        // если была написана команда who
        else if (strcmp(word, "who") == 0)
        {
            // перебираем по-индексно всех клиентов и выводим информацию о них
            for (int i = 0; i < socketList.size(); i++) {
                getpeername(socketList.at(i), (sockaddr*)&addr, &clientAddrSize);
                osyncstream(cout) << "\rINFO: Client [" << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port) <<
                    "] with socket [" << socketList.at(i) << "] is served by thread [" << threadList.at(i).get_id() << ']' << endl;
            }

            // если вектор сокетов пуст — значит на сервере никого нет
            if (socketList.size() == 0) {
                osyncstream(cout) << "\rINFO: there is no one on the server right now, but it doesn't matter, someone will definitely join" << endl;
            }
        }

        // если была написана команда stop
        else if (strcmp(word, "stop") == 0)
        {
            // опустим флаг готовности работы программы
            flagReady = 0;

            // освобождаем все потоки от основной программы
            for (auto& thread : threadList)
            {
                thread.detach();
            }

            // перебираем сокеты всех клиентов
            for (auto& curSocket : socketList)
            {
                // получаем данные о клиентах через их сокет
                getpeername(curSocket, (sockaddr*)&addr, &clientAddrSize);
                // выводим сообщение
                osyncstream(cout) << "\rINFO: Client [" << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port)
                    << "] is disconnected from the server." << endl;
                // завершаем работу с текущим сокетом
                WinSockOff(curSocket, 1, 0, NULL);
            }

            // очищаем векторы сокетов и потоков
            threadList.clear();
            socketList.clear();

            // завершаем все процессы работы сервера
            WinSockOff(listeningSocket, 0, 1, NULL);

            // завершим работу потока
            return;
        }
        else
        {
            // если админ ввел неправильную команду, скажем ему об этом
            osyncstream(cout) << "\rUnknown command, write 'help' to see the list of available commands." << endl;
        }
    }
}

// работа с клиентами
void doExchange(SOCKET s, sockaddr_in addr)
{
    // получаем ID данного потока
    jthread::id currentThreadId = this_thread::get_id();

    // заводим 2 символьных массива для корректной пересылки данных о других пользователях
    char port_str[6];       // Максимальная длина порта - 5 символов + 1 символ для завершающего нуля
    char socket_str[17];    // Максимальная длина сокета для IPv4 - 16 символов + 1 символ для завершающего нуля

    // создаем переменные для работы с поступившими данными
    char* word;
    const unsigned short int bufferSize = 1024;
    unsigned short int clientID = 0;
    string sendMessage;

    while (1)
    {
        // динамически выделяем память под прием сообщения
        word = new char[bufferSize];

        // если пришло больше 0 байт, то работаем с поступившей информацией
        if (recv(s, word, bufferSize, 0) > 0)
        {
            // если пришла команда bye
            if (strcmp(word, "bye") == 0) break;

            // если пришла команда who
            else if (strcmp(word, "help") == 0)
            {
                // блокируем доступ другим потокам к отправке сообщения через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
                lock_guard<mutex> guard(mutexSend);

                // формируем сообщение для отправки
                sendMessage = "help - shows a list of available commands\n"
                    "bye - to break the connection with the server\n"
                    "who - shows current connections\n"
                    "ping - receive 20 packets from the server indicating the response rate in milliseconds\n"
                    "cd 'path' - changing the directory on the server\n"
                    "pwd - shows the current directory on the server\n"
                    "ls - shows a list of files in the current server directory";

                // отправляем сообщение клиенту
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            // если пришла команда who
            else if (strcmp(word, "who") == 0)
            {
                // блокируем доступ другим потокам к отправке сообщения через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
                lock_guard<mutex> guard(mutexSend);

                // инициализируем структуру и ее размер для получения и вывода данных о клиентах
                sockaddr_in addr;
                int clientAddrSize = sizeof(addr);
                // подготавливаем переменную для отображения
                sendMessage = '\r';

                for (const auto& socked : socketList)
                {
                    // получаем данные о клиентах через их сокет
                    getpeername(socked, (sockaddr*)&addr, &clientAddrSize);
                    snprintf(port_str, sizeof(port_str), "%i", ntohs(addr.sin_port));
                    snprintf(socket_str, sizeof(socket_str), "%llu", socked);

                    // формируем сообщение для отправки
                    sendMessage += "Client [";
                    sendMessage += inet_ntoa(addr.sin_addr);
                    sendMessage += ':';
                    sendMessage += port_str;
                    sendMessage += "] with socket [";
                    sendMessage += socket_str;
                    sendMessage += "] is online.\n";
                }
                // добовляем признак окончания списка
                sendMessage += "END_OF_LIST_USERS";
                // отправляем сообщение клиенту
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            // если пришла команда pwd
            else if (strcmp(word, "pwd") == 0)
            {
                // блокируем доступ другим потокам к отправке сообщения через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
                lock_guard<mutex> guard(mutexSend);

                // Получаем путь текущей директории в виде string
                sendMessage = current_path().string();

                // Отправляем клиенту путь текущей директории
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            // если пришла команда cd...
            else if (strstr(word, "cd"))
            {
                // блокируем доступ другим потокам к отправке сообщения через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
                lock_guard<mutex> guard(mutexSend);

                // будем отлавливать ошибки, в основном они будут связаны с неправильно написанной командой или путем
                try
                {
                    // извлекаем из команды аргумент - директорию
                    string directory = string(word + 3);

                    // переходим в заданную директорию
                    current_path(directory);

                    // подготавливаем текст к отправке клиенту
                    sendMessage = "Directory has been successfully changed to ";
                    sendMessage += directory;

                    // отправляем клиенту сообщение об успешном переходе
                    send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
                }
                catch (const exception& e)
                {
                    // вывод сообщение через поток ошибок о ошибке во время смены дериктории
                    cerr << "\rSERVER: Unexpected error with the code:" << e.what() << endl;
                    // в случае ошибки отправляем клиенту, что сменить директорию не удалось
                    send(s, "Unable to change directory", sizeof("Unable to change directory"), 0);
                }
            }


            // если пришла команда ls
            else if (strcmp(word, "ls") == 0)
            {
                // блокируем доступ другим потокам к отправке сообщения через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
                lock_guard<mutex> guard(mutexSend);

                // инициализируем итератор, который будет указывать на файлы текущей директории
                directory_iterator cursor(".");

                // инициализируем переменную, в которой будет храниться список файлов (пробелы нужны, чтобы перекрыть слово Recived)
                sendMessage = '\r';

                // перебираем все элементы, до которых сможет добраться итератор (все файлы текущей директории)
                for (const auto& entry : cursor)
                {
                    // сохраняем в переменную путь и название файла в виде строки
                    string filename = entry.path().filename().string();
                    sendMessage += filename;
                    sendMessage += '\n';
                }
                // добовляем признак окончания списка
                sendMessage += "END_OF_LIST_FILES";

                // оправляем клиенту список файлов в текущей директории в виде строки в которую записан путь и имя файла
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            else if (strcmp(word, "ping") == 0)
            {
                // блокируем доступ другим потокам к отправке сообщения через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
                lock_guard<mutex> guard(mutexSend);

                // устанавливаем точку для расчета рандома в значение текущего системного времени
                srand(static_cast<unsigned int>(time(NULL)));

                // отправляем 20 пакетов
                for (int i = 0; i < 20; i++)
                {   // имитируем задержку и отправляем служебный пакет для замера скорости (увеличение пинга от 100мс до 199мс)
                    Sleep((rand() % 100) + 100);
                    send(s, "0x762f62ff", sizeof("0x762f62ff"), 0);
                }

                // устанавливаем задержку для успешной отправки пакета, которая заканчивает проверку скорости
                Sleep(100);
                send(s, "stop", sizeof("stop"), 0);

                // используем задержку в 100мс, чтобы избежать потери данных при передаче сообщения о успешном завершении проверки скорости
                Sleep(100);
                send(s, "\rINFO: Pheck completed", sizeof("\rINFO: Pheck completed"), 0);
            }

            else
            {
                // блокируем доступ другим потокам к отправке сообщения через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
                lock_guard<mutex> guard(mutexSend);

                // подготавливаем текст к отправке клиенту
                string sendMessage = "Unknown command, write 'help' to see the list of available commands.";

                // отправляем клиенту сообщение о неккоректной команде
                send(s, sendMessage.c_str(), static_cast<int>(sendMessage.length()) + 1, 0);
            }

            // если пришла не команда, то отображаем на экране адрес и порт отправителя, а также текст сообщения
            // используем класс osyncstream для реализации потокобезопасного вывода в консоль
            osyncstream(cout) << "\rINFO: Received from " << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port) << " - " << word << endl;
            osyncstream(cout) << "\033[32m>\033[0m ";
        }
        // если клиент отключился или пришла битая информация переходим на разрыв соединения с ним
        else break;

        // очищаем буфер под принятое сообщение (это быстрее, чем очищать информацию)
        delete[] word;

        // уходим на новую итерацию если не было команды bye и пришло > 0 байт от клиента
    }

    // если сервер находится в рабочем состоянии, но потеряна связь с клиентом, то отключаем его
    if (flagReady)
    {
        // блокируем доступ другим потокам работе с векторами через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
        {
            lock_guard<mutex> guard(mutexThreadList);

            // перебираем все активные потоки пока не найдем позицию текущего
            for (int i = 0; i < threadList.size(); i++) {
                if (currentThreadId == threadList.at(i).get_id()) {
                    clientID = i;
                    break;
                }
            }

            // удаляем сокет данного клиента из вектора сокетов
            socketList.erase(socketList.begin() + clientID);
            // уменьшаем счетчик активных клиентов
            clientCounter--;
            // отсоединяем поток от основной программы
            threadList.at(clientID).detach();
            // удаляем поток из вектора потоков, тем самым закрывая его
            threadList.erase(threadList.begin() + clientID);
        }

        // завершаем работу с определенным сокетом
        WinSockOff(s, 1, 0, NULL);

        // очистка ресурсов
        delete[] word;

        // вывод информации
        osyncstream(cout) << "\rSERVER: User [" << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port) << "] disconect from server" << endl;
        osyncstream(cout) << "\033[32m>\033[0m ";
    }
}

// основная функция
int main()
{
    // заводим переменную под сокет для работы с клиентами
    SOCKET clientSocket;

    // структура для хранения адресов хостов
    struct sockaddr_in hostAddres;

    // инициализируем переменную как размер структуры, в которую будут помещаться информация о данных хостов
    int addrSize = sizeof(sockaddr_in);



    // инициализируем WinSock, если инициализация не удалась, то выходим с кодом -1
    if (WinSockInit()) return -1;



    // инициализируем "слушающий" сокет для приема клиентов, если функция вернула INVALID_SOCKET, то выходим с ошибкой -2
    if ((listeningSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        // выводим сообщение об ошибке
        cout << "\rSERVER: Error creating listening socket. Error code: " << WSAGetLastError() << endl;
        // отключаем WinSock
        WSACleanup();
        // выводим сообщение о закрытии сервера
        cout << "\rSERVER: WinSock was successfully closed." << endl;
        // выходим с ошибкой
        return -2;
    }
    // иначе в "слушающем" сокете оказывается его дескриптор, выводим его на экран
    else cout << "\rSERVER: Listening socket [" << listeningSocket << "] successfully created." << endl;



    // заводим поток для серверных команд
    jthread commands(serverCommands, listeningSocket);

    // задаем параметры для будущих подключений
    hostAddres.sin_addr.s_addr = INADDR_ANY;  // любой IP-адрес
    hostAddres.sin_family = AF_INET;          // IPv4
    hostAddres.sin_port = ServerPort;         // порт

    // связываем параметры со «слушающим» сокетом, если привязка не удалась выходим с ошибкой -2
    if (bind(listeningSocket, (sockaddr*)&hostAddres, sizeof(hostAddres)))
    {
        // выводим информацию об ошибке на экран
        cout << "\rSERVER: Error binding to hostAddres [" << inet_ntoa(hostAddres.sin_addr) << ':' << ntohs(ServerPort) << "] Error code : " << WSAGetLastError() << endl;
        // завершаем работу со слушающим сокетом
        WinSockOff(listeningSocket, 0, 1, -3);
    }
    // иначе настройки сети успешно привязаны к сокету, выводим сообщение на экран
    else
    {
        // выводим данные по "слушающему" сокету на экран
        cout << "\rSERVER: HostAddres [" << inet_ntoa(hostAddres.sin_addr) << ':' << ntohs(ServerPort) <<
            "] successfully bound to socket [" << listeningSocket << ']' << endl;
    }



    // устанавливаем "слушающий" сокет в режим "прослушивания" для максимального количества подключений доступных в системе
    if (listen(listeningSocket, maxCount))
    {
        // выводим информацию об ошибке на экран
        cout << "\rSERVER: Error listening on socket. Error code: " << WSAGetLastError() << endl;
        // завершаем работу со слушающим сокетом
        WinSockOff(listeningSocket, 0, 1, -4);
    }
    // иначе "прослушка" успешно началась, выводим сообщение на экран
    else
    {
        // выводим данные по "слушающему" сокету на экран
        cout << "\rSERVER: Socket [" << listeningSocket << "] has started listening for incoming connections." << endl <<
            "SERVER: Maximum number of clients on the server [" << maxCount << ']' << endl;
    }



    // выводим сообщение об ожидании клиентов и сообщение о возможностях сервера
    cout << "\rSERVER: Waiting for clients to connect..." << endl <<
        "SERVER: Type 'help' to view the available commands." << endl;

    // обозначаем, что сервер готов к работе
    flagReady = 1;

    // создаем метку для возобновления возможности принятия клиентов
waiting:
    // бесконечный цикл по обработке подключений
    while (1)
    {
        // будем контролировать возникающие исключения при подключении новых клиентов
        try
        {
            // принимаем соединение, если функция вернула INVALID_SOCKET, то выводим сообщение об ошибке и пропускаем данного клиента
            if (((clientSocket = accept(listeningSocket, (sockaddr*)&hostAddres, &addrSize)) == INVALID_SOCKET))
            {
                // если сервер выключен, то выход из цикла и завершения программы
                if (!flagReady) break;
                // используем класс osyncstream для реализации потокобезопасного вывода в консоль
                osyncstream(cout) << "\rSERVER: Error accepting connection. Error code: " << WSAGetLastError() << endl;
                continue;
            }
            else
            {
                // если сервер заполнен отключаем всех новых клиентов
                if (clientCounter >= maxCount)
                {
                    // выводим сообщение о том, что к нам хотели подключиться, но свободных мест нет
                    // используем класс osyncstream для реализации потокобезопасного вывода в консоль
                    osyncstream(cout) << "\rSERVER: Client [" << inet_ntoa(hostAddres.sin_addr) << ':'
                        << ntohs(hostAddres.sin_port) << "] and socket [" << clientSocket << "] tried to connect, but "
                        << "the server is full" << endl << flush << "\033[32m>\033[0m " << flush;

                    WinSockOff(clientSocket, 1, 0, NULL);
                    continue;
                }

                // если подключение прошло успешно, то выводим на экран данные о пользователе
                // используем класс osyncstream для реализации потокобезопасного вывода в консоль
                osyncstream(cout) << "\rSERVER: Connection accepted from [" << inet_ntoa(hostAddres.sin_addr) << ':'
                    << ntohs(hostAddres.sin_port) << "] and socket [" << clientSocket << ']' << endl << flush;
                osyncstream(cout) << "\033[32m>\033[0m " << flush;

                {
                    // блокируем доступ другим потокам работе с векторами через lock_guard (в конструкторе вызывает lock, а в деструкторе unlock)
                    lock_guard<mutex> guard(mutexThreadList);

                    // добавляем сокет в список (push_back т.к. добавляем существующий объект в ячейку вектора)
                    socketList.push_back(clientSocket);
                    // создаем новый поток для обслуживания этого клиента (emplace_back т.к. создаем новый объект в ячейке вектора)
                    threadList.emplace_back(jthread(doExchange, clientSocket, hostAddres));
                }
                // увеличиваем счетчик подключений
                clientCounter++;
            }
        }
        // если возникают какие-то исключения ловим их и пропускаем данного клиента 
        catch (const system_error& e)
        {
            // вывод сообщение через поток ошибок о непредвиденной ошибке
            cerr << "\rSERVER: Unexpected error with the code:" << e.what() << endl;
            continue;
        }
    }
    // при отключении клиентов их потоки закрываются, поэтому ждать их нет смысла



    // пока сервер работаем сидим на обслуживании подключенных клиентов
    while (flagReady)
    {
        // если на сервере появляется место, то переходим на подключение новых клиентов
        if (clientCounter < maxCount) goto waiting;
    }



    // выводим сообщение о закрытии сервера
    cout << "\rSERVER: The server has been successfully stopped." << endl;

    // освобождаем поток для серверных команд
    commands.detach();

    // если сервер дошел до данной точки, значит его остановили командой stop и все действия по отключению клиентов, закрытию сокетов и т.д. выполнены
    return 0;
}
