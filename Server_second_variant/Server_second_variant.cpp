// Отключение предупреждения с кодом 4996
#pragma warning(disable : 4996)
// Включение библиотеки ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

// Подключение необходимых заголовочных файлов
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

// Использование пространств имен std и filesystem
using namespace std;
using namespace filesystem;

// Определение максимального размера буфера
const int MAX_BUFFER_SIZE = 8192;
// Хранение сокетов каждого клиента
unordered_map<SOCKET, string> clientSockets;
// Мьютекс для синхронизации доступа к хранилищу сокетов
mutex clientSocketsMutex;

// Функция для выполнения команды, полученной от клиента
string executeCommand(const string& command, SOCKET clientSocket)
{
	// Переменная для хранения результата выполнения команды
	string result;

	// Вектор для хранения токенов команды
	vector<string> tokens;
	// Создание потока ввода из строки команды
	istringstream iss(command);
	// Переменная для хранения текущего токена
	string token;

	// Проверка наличия команды mPUT в запросе
	if (command.find("mPUT|") != string::npos) {
		// Разбиение команды на токены с использованием разделителя '|'
		while (getline(iss, token, '|'))
		{
			tokens.push_back(token);
		}
	}
	else {
		// Разбиение команды на токены с использованием разделителя ' '
		while (getline(iss, token, ' '))
		{
			tokens.push_back(token);
		}
	}

	// Вывод информации о полученной команде в зависимости от количества токенов
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
		cout << "Невозможно отобразить полученную команду" << endl;
	}

	// Обработка команды ls
	if (tokens[0] == "ls")
	{
		// Проверка корректности формата команды ls
		if (tokens.size() != 1)
		{
			result = "Ошибка: некорректный формат команды ls\n";
		}
		else
		{
			// Получение текущего пути каталога
			string currentPath = filesystem::current_path().string();
			result = "Содержимое текущего каталога:\n";

			// Перебор содержимого текущего каталога и добавление в результат
			for (const auto& entry : filesystem::directory_iterator(currentPath))
			{
				result += entry.path().filename().string() + "\n";
			}
		}
	}
	// Обработка команды mPUT
	else if (tokens[0] == "mPUT")
	{
		// Проверка корректности формата команды mPUT
		if (tokens.size() != 4)
		{
			result = "Ошибка: некорректный формат команды mPUT\n";
		}
		else
		{
			// Получение имени файла, размера файла и данных файла из токенов
			string fileName = tokens[1];
			int fileSize = stoi(tokens[2]);
			string fileData = tokens[3];

			// Открытие файла для записи в бинарном режиме
			ofstream file(fileName, ios::binary);
			if (!file.is_open())
			{
				result = "Ошибка при создании файла на сервере\n";
			}
			else
			{
				// Запись данных файла в файл на сервере
				file.write(fileData.c_str(), fileSize);
				file.close();
				result = "Файл успешно скопирован на сервер\n";
			}
		}
	}
	// Обработка команды pwd
	else if (tokens[0] == "pwd")
	{
		// Проверка корректности формата команды pwd
		if (tokens.size() != 1)
		{
			result = "Ошибка: некорректный формат команды pwd\n";
		}
		else
		{
			// Получение текущего пути каталога
			string currentPath = filesystem::current_path().string();
			result = "Текущий путь каталога: " + currentPath + "\n";
		}
	}
	// Обработка команды bye
	else if (tokens[0] == "bye")
	{
		// Проверка корректности формата команды bye
		if (tokens.size() != 1)
		{
			result = "Ошибка: некорректный формат команды bye\n";
		}
		else
		{
			result = "Отключение от сервера...\n";
		}
	}
	// Обработка команды help
	else if (tokens[0] == "help")
	{
		// Проверка корректности формата команды help
		if (tokens.size() > 2)
		{
			result = "Ошибка: некорректный формат команды help\n";
		}
		else
		{
			// Вывод списка доступных команд, если команда help без аргументов
			if (tokens.size() == 1)
			{
				result = "Список доступных команд: ls, pwd, mPUT, bye, help\n"
					"\t   Для получения дополнительной информации по каждой из команд введите 'help <команда>'\n";
			}
			else
			{
				// Вывод описания конкретной команды, если указан аргумент
				if (tokens[1] == "ls")
				{
					result = "Вывод содержимого текущего каталога на сервере\n";
				}
				else if (tokens[1] == "pwd")
				{
					result = "Вывод пути текущей дирректории на сервере\n";
				}
				else if (tokens[1] == "mPUT")
				{
					result = "Копирование файла с локального устройства на сервер\n"
						"\t   шаблон mPUT <имя_файла/путь_файла>\n";
				}
				else if (tokens[1] == "bye")
				{
					result = "Завершение сеанса с сервером\n";
				}
				else if (tokens[1] == "help")
				{
					result = "Вывод информации о доступных командах\n";
				}
				else
				{
					result = "Ошибка: некорректный формат команды help\n";
				}
			}
		}
	}
	// Обработка неизвестной команды
	else
	{
		result = "Неизвестная команда\n";
	}

	// Возврат результата выполнения команды
	return result;
}

// Функция для обработки клиентского подключения
void clientHandler(SOCKET clientSocket)
{
	// Буфер для приема данных от клиента
	char buffer[MAX_BUFFER_SIZE];

	// Структура для хранения информации об адресе клиента
	sockaddr_in clientAddress{};
	int clientAddressSize = sizeof(clientAddress);
	// Получение информации об адресе клиента
	getpeername(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
	// Получение IP-адреса клиента
	string clientIP = inet_ntoa(clientAddress.sin_addr);
	// Получение порта клиента
	int clientPort = ntohs(clientAddress.sin_port);

	// Вывод информации о подключении клиента
	cout << "Клиент подключен [" << clientIP << ":" << clientPort << "]" << endl;

	// Цикл обработки сообщений от клиента
	while (true)
	{
		// Получение данных от клиента
		int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
		// Проверка на ошибку или отключение клиента
		if (bytesRead == SOCKET_ERROR || bytesRead == 0)
		{
			// Блокировка мьютекса для доступа к хранилищу сокетов
			lock_guard<mutex> lock(clientSocketsMutex);
			// Удаление сокета клиента из хранилища
			clientSockets.erase(clientSocket);
			// Вывод информации об отключении клиента
			cout << "Клиент отключен [" << clientIP << ":" << clientPort << "]" << endl;
			break;
		}

		// Преобразование полученных данных в строку
		string clientMessage(buffer, bytesRead);
		// Вывод информации о полученном сообщении от клиента
		cout << "От клиента [" << clientIP << ":" << clientPort << "] - ";

		// Выполнение команды, полученной от клиента
		string result = executeCommand(clientMessage, clientSocket);

		// Блокировка мьютекса для доступа к хранилищу сокетов
		{
			lock_guard<mutex> lock(clientSocketsMutex);
			// Сохранение результата выполнения команды для отправки клиенту
			clientSockets[clientSocket] = result;
		}
	}

	// Закрытие сокета клиента
	closesocket(clientSocket);
}

// Функция для отправки результата выполнения команды клиенту
void sendResultToClient(SOCKET clientSocket)
{
	// Бесконечный цикл отправки результатов клиенту
	while (true)
	{
		// Переменная для хранения результата выполнения команды
		string result;

		// Блокировка мьютекса для доступа к хранилищу сокетов
		{
			lock_guard<mutex> lock(clientSocketsMutex);
			// Проверка наличия результата для данного клиента в хранилище
			if (clientSockets.find(clientSocket) != clientSockets.end())
			{
				// Получение результата выполнения команды для данного клиента
				result = clientSockets[clientSocket];
				// Удаление результата из хранилища
				clientSockets.erase(clientSocket);
			}
		}

		// Отправка результата клиенту, если он не пустой
		if (!result.empty())
		{
			send(clientSocket, result.c_str(), result.length(), 0);
		}

		// Задержка перед следующей итерацией цикла
		this_thread::sleep_for(chrono::milliseconds(100));
	}
}

// Главная функция программы
int main()
{
	// Установка русской локали
	setlocale(LC_ALL, "ru");

	// Инициализация Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cerr << "Ошибка инициализации Winsock" << endl;
		return 1;
	}

	// Создание серверного сокета
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		cerr << "Ошибка создания сокета" << endl;
		WSACleanup();
		return 1;
	}

	// Настройка адреса сервера
	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(8080);

	// Привязка серверного сокета к адресу
	if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cerr << "Ошибка при привязке сокета к адресу" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// Перевод серверного сокета в режим прослушивания
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "Ошибка при прослушивании входящих подключений" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// Вывод информации о запуске сервера
	cout << "Сервер запущен. Ожидание подключений..." << endl;

	// Бесконечный цикл обработки входящих подключений
	while (true)
	{
		// Принятие входящего подключения
		SOCKET clientSocket = accept(serverSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "Ошибка при принятии входящего подключения" << endl;
			closesocket(serverSocket);
			WSACleanup();
			return 1;
		}

		// Создание потока для обработки клиентского подключения
		thread clientThread(clientHandler, clientSocket);
		clientThread.detach();

		// Создание потока для отправки результатов клиенту
		thread sendThread(sendResultToClient, clientSocket);
		sendThread.detach();
	}

	// Закрытие серверного сокета
	closesocket(serverSocket);
	// Очистка Winsock
	WSACleanup();

	// Завершение программы
	return 0;
}
