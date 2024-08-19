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

// Использование пространств имен std и filesystem
using namespace std;
using namespace filesystem;

// Определение максимального размера буфера
const int MAX_BUFFER_SIZE = 8192;

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

	// Создание клиентского сокета
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		cerr << "Ошибка создания сокета" << endl;
		WSACleanup();
		return 1;
	}

	// Настройка адреса сервера
	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr("172.20.10.4");
	serverAddress.sin_port = htons(8080);

	// Подключение к серверу
	if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cerr << "Ошибка подключения к серверу" << endl;
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	// Бесконечный цикл обработки команд
	while (true)
	{
		// Ввод команды с клавиатуры
		cout << "Введите команду: ";
		string command;
		getline(cin, command);

		// Проверка на команду mPUT (кроме help)
		if (strstr(command.c_str(), "mPUT") and !strstr(command.c_str(), "help"))
		{
			// Разбиение команды на токены
			vector<string> tokens;
			istringstream iss(command);
			string token;
			while (getline(iss, token, ' '))
			{
				tokens.push_back(token);
			}
			// Проверка корректности формата команды mPUT
			if (tokens.size() != 2)
			{
				cout << "Ошибка: некорректный формат команды mPUT\n" << endl;
				continue;
			}

			// Открытие файла для чтения в бинарном режиме
			ifstream file(tokens[1], ios::binary);
			if (!file.is_open())
			{
				cout << "Файл не может быть открыт!\n" << endl;
				continue;
			}
			else
			{
				// Получение размера файла
				int fileSize = file_size(tokens[1]);
				// Выделение памяти для хранения данных файла
				char* data = new char[fileSize];

				// Чтение данных файла
				file.read(data, fileSize);

				// Формирование команды mPUT с именем файла, размером и данными
				command = "mPUT|" + tokens[1] + "|" + to_string(fileSize) + "|";
				command.append(data, fileSize);

				// Освобождение памяти и закрытие файла
				delete[] data;
				file.close();
			}
		}

		// Отправка команды серверу
		if (send(clientSocket, command.c_str(), command.length(), 0) == SOCKET_ERROR)
		{
			cerr << "Ошибка при отправке данных серверу" << endl;
			break;
		}

		// Получение ответа от сервера
		char buffer[MAX_BUFFER_SIZE];
		int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesRead == SOCKET_ERROR || bytesRead == 0)
		{
			cerr << "Ошибка при получении данных от сервера" << endl;
			break;
		}

		// Вывод результата на экран
		string result(buffer, bytesRead);
		cout << "Результат: " << result << endl;

		// Проверка на команду bye для завершения работы
		if (command == "bye")
		{
			break;
		}
	}

	// Закрытие клиентского сокета
	closesocket(clientSocket);
	// Очистка Winsock
	WSACleanup();

	// Завершение программы
	return 0;
}
