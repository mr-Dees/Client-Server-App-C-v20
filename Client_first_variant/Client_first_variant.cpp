#include <iostream>         // стандартная библиотека ввода-вывода
#include <winsock2.h>       // заголовочный файл для работы с сокетами в windows
#include <ws2tcpip.h>       // заголовочный файл с дополнительными функциями по работе с сокетами в windows
#include <vector>           // библиотека для удобного размещения потоков и данных о клиентах
#include <string>           // библиотека для работы со строками
#include <thread>           // библиотека для осуществления многопоточности (jthread появились в стандарте ISO C++ 20)
#include <algorithm>	    // библиотека для работы с контейнерами STL (нам нужен только метод transform())
#include <cctype>	    // библиотека для работы с символами (нам нужен только метод tolower())


// через директиву препроцессора подключаем библиотеку WinSock 
#pragma comment(lib, "ws2_32.lib")
// через директиву препроцессора отключаем ошибки, связанные с использованием устаревших функций (C4996)
#pragma warning(disable : 4996)


// объявляем пространство имен std, чтобы не писать соответствующий префикс std::
using namespace std;
double start_time = 0, end_time = 0;	// переменные для замера времени
bool flag_ping = 0;						// флаг для определения пакетов пинга

//==================================================================================================//
const unsigned int ServerIP = inet_addr("172.20.10.4");		// IPv4 адрес локальной сети сервера	//
const unsigned short int ServerPort = htons(83);			// номер порта							//
//==================================================================================================//


// Инициализация Windows Sockets DLL
int WinSockInit()
{
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;

	// выводим сообщение о старте DLL
	cout << "INFO: Starting WinSock..." << endl;

	// Проинициализируем WinSock, если возникли ошибки, то отключаем DLL и выходим из функции с кодом 1
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		cout << "INFO: Error: Couldn't find a usable WinSock DLL." << endl;
		WSACleanup();
		return 1;
	}

	// Проверка версии WinSock, если версия не 2.2, то отключаем DLL и выходим из функции с кодом 1
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		cout << "INFO: Error: Couldn't find a usable WinSock DLL." << endl;
		WSACleanup();
		return 1;
	}

	// Если инициализация и проверка версии не вызвали ошибок, то выводим сообщение и возвращаем значение 0
	cout << "INFO: WinSock started successfully." << endl;
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
			cout << "\rINFO: Data transmission stopped." << endl;
			// выводим сообщение о закрытии сокета
			cout << "\rINFO: Listening socket [" << socket << "] successfully closed." << endl;
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
			cout << "\rINFO: Data transmission stopped." << endl;
			// выводим сообщение о закрытии сокета
			cout << "\rINFO: Listening socket [" << socket << "] successfully closed." << endl;
			// выводим сообщение о закрытии сервера
			cout << "\rINFO: WinSock was successfully closed." << endl;
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


// Передача данных
void doSend(SOCKET s)
{
	// инициализируем текущий сокет и массив под хранение строки введенной пользователем
	char word[1024];
	string str;

	// бесконечный цикл
	while (1)
	{
		// выводим символ-приглашение
		cout << "\033[32m>\033[0m ";
		// запрашиваем у пользователя ввод строки, результат помещаем в буфер word. Максимальная длина 1024 символа.
		cin.getline(word, 1024);
		// копируем массив символов в строку для удобной работы
		str = word;

		// проверяем сколько было введено символов и трансформируем их в строчные буквы для корректности обработки команды
		if (str.length() == 2) transform(str.begin(), str.begin() + 2, str.begin(), [](unsigned char c) { return tolower(c); });
		if (str.length() == 3) transform(str.begin(), str.begin() + 3, str.begin(), [](unsigned char c) { return tolower(c); });
		if (str.length() == 4) transform(str.begin(), str.begin() + 4, str.begin(), [](unsigned char c) { return tolower(c); });

		// если клиент написал bye, значит он хочет отключиться от сервера. Выходим из цикла и делаем это
		if (strcmp(str.c_str(), "bye") == 0) break;

		// если клиент написал ping, значит хочет получить пинг от сервера, приводим его устройства в режим ожидания пакетов
		if (strcmp(str.c_str(), "ping") == 0)
		{
			// вывод информационного сообщения
			cout << "\rINFO: You will be sent 20 packets to measure the connection speed" << endl;

			// включаем режим принятия серии пакетов пинга
			flag_ping = 1;
			// засекаем время
			double ticks = clock();
			start_time = ticks / CLOCKS_PER_SEC * 1000.0;
		}

		// отправляем полученные от пользователя данные из буфера word, длинной введенных символов +1 
		// (чтобы учесть символ признака окончания строки т.к. strlen не учитывает '\0')
		send(s, str.c_str(), static_cast<int>(str.length()) + 1, 0);

		// уходим на новую итерацию если не было команды bye
	}

	// отправляем команду завершения связи из буфера word, длинной +1 
	// (чтобы учесть символ признака окончания строки т.к. strlen не учитывает '\0')
	send(s, str.c_str(), static_cast<int>(str.length()) + 1, 0);

	// выход из функции
	return;
}
// Прием данных
void doReceiv(SOCKET s)
{
	// создаем переменные для работы с поступившими данными
	char* word;
	const unsigned short int bufferSize = 1024;

	while (1)
	{
		// динамически выделяем память под прием сообщения
		word = new char[bufferSize];

		// если пришло больше 0 байт, то работаем с поступившей информацией
		if (recv(s, word, bufferSize, 0) > 0)
		{
			// если мы принимаем серию пакетов, то все сообщения кроме stop воспринимаем как пакет для замера
			if (flag_ping)
			{
				// если пришла команда stop
				if (strcmp(word, "stop") == 0)
				{
					// выходим из режима обработки пакетов пинга
					flag_ping = 0;
					// переходим к приему нового сообщения
					continue;
				}
				// если пришел пакет
				else
				{
					// замеряем время прибытия последнего пакета пинга
					double ticks = clock();
					end_time = ticks / CLOCKS_PER_SEC * 1000.0;

					// высчитываем разницу во времени между пакетами пинга
					double result = end_time - start_time;
					// если время больше 2.5 секунд
					if (result > 2500)
					{
						// выводим сообщение о том, что время ожидания превышено
						cout << "\rINFO: Waiting time exceeded: " << endl;
						// выходим из режима обработки пакетов пинга 
						flag_ping = 0;
						// переходим к приему нового сообщения
						continue;
					}
					// если время между пакетами меньше 2.5 секунд
					else
					{
						// выводим сколько заняла отправка и получение пакета пинга
						cout << "\rping: " << result << endl;

						// засекаем время до получения нового пакета пинга
						double ticks = clock();
						start_time = ticks / CLOCKS_PER_SEC * 1000.0;
						// переходим к приему нового пакета
						continue;
					}
				}
			}

			// поскольку от сервера не может прийти команды bye, просто выводим результат наших запросов
			cout << '\r' << word << endl;
			// выводим символ-приглашение
			cout << "\033[32m>\033[0m ";
		}
		// если связь с сервером потеряна или пришла битая информация переходим от разрыв соединения с ним
		else break;

		// очищаем память перед очередным заходом в цикл
		delete[] word;
	}

	// очищаем память перед выходом из функции (если была вызвана команда break, то delete на 174 не выполнится)
	delete[] word;

	// выход из функции
	return;
}


// основная функция
int main()
{
	// заводим переменную под сокет
	SOCKET listeningSocket;

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
		cout << "\rINFO: Error creating listening socket. Error code: " << WSAGetLastError() << endl;
		// отключаем WinSock
		WSACleanup();
		// выводим сообщение о закрытии сервера
		cout << "\rINFO: WinSock was successfully closed." << endl;
		// выходим с ошибкой
		return -2;
	}
	// иначе в "слушающем" сокете оказывается его дескриптор, выводим его на экран
	else cout << "\rINFO: Listening socket [" << listeningSocket << "] successfully created." << endl;



	// Установка параметров сокета (IPv4, порт, IP-адрес)
	hostAddres.sin_family = AF_INET;
	hostAddres.sin_port = ServerPort;
	hostAddres.sin_addr.s_addr = ServerIP;

	// Попробуем соединиться с указанным компьютером, если не получилось то
	if (connect(listeningSocket, (sockaddr*)&hostAddres, sizeof(hostAddres)))
	{
		// выводим сообщение о том, что сервер для подключения не найден
		cout << "\rINFO: Server not found. Error code: " << WSAGetLastError() << endl;
		// завершаем работу со слушающим сокетом
		WinSockOff(listeningSocket, 0, 1, -3);
	}



	// создаем два потока на отправку и получение данных
	thread rec(doReceiv, listeningSocket);
	thread snd(doSend, listeningSocket);



	// очищаем консоль
	system("cls");
	// выводим сообщение о успешном подключении и подсказку 'help'
	cout << "\rINFO: The connection is successful, enter 'help' to view the available commands." << endl;
	// выводим символ-приглашение
	cout << "\033[32m>\033[0m ";

	// ждем пока поток приема не закончит работу (пока не разорвано соединение с сервером)
	rec.join();
	snd.detach();



	// когда потоки завершили свою работу выводим сообщение, что сеанс завершен
	cout << "\rINFO: You have been disconnected from the server." << endl;

	// завершаем работу со слушающим сокетом
	WinSockOff(listeningSocket, 0, 1, -4);

	// выходим победителями
	return 0;
}
