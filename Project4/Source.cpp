#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <fstream>

// Для корректной работы freeaddrinfo в MinGW
// Подробнее: http://stackoverflow.com/a/20306451
#define _WIN32_WINNT 0x501

#include <WinSock2.h>
#include <WS2tcpip.h>

// Необходимо, чтобы линковка происходила с DLL-библиотекой
// Для работы с сокетам
#pragma comment(lib, "Ws2_32.lib")

using std::cerr;
using namespace std;

struct message {
	string inf;
	string user;
	string time;
	message* next;
};

void print_list_console(message* head);

string replace(string gde, char chto, char na_chto) {
	for (int i = 0; i < gde.length(); ++i) {
		if (gde[i] == chto) {
			gde[i] = na_chto;
		}
	}
	return gde;
}

string leveling(string gde) {
	for (int i = 0; i < gde.length(); ++i) {
		if (gde[i] == '\n') {
			gde.erase(i, 1);
		}
	}
	return gde;
}

void print_list_console(message* head) {

	for (message* iterator = head; iterator != NULL; iterator = iterator->next) {
		cout << iterator->time << " " << iterator->user << " --> " << iterator->inf << "...\n";
	}
}

string print_list_browser(message* head) {
	string res = "";
	for (message* iterator = head; iterator != NULL; iterator = iterator->next) {
		res += iterator->time + " <strong>" + iterator->user + "</strong> --> " + iterator->inf + "...<br>";
	}
	return res;
}

message* clear(message* head) {
	message* prev = head;
	for (message* deleter = head->next; deleter != NULL; prev = deleter, deleter = deleter->next) {
		if (prev != head) {
			free(prev);
		}
	}
	head->next = NULL;
	return head;
}

message* add_message(message* head, string info, string user) {
	message* new_message = new message();
	new_message->inf = info;
	new_message->user = user;
	new_message->next = head;
	time_t date = time(NULL);
	tm* datetime = localtime(&date);
	new_message->time = leveling(asctime(datetime));
	head = new_message;
	return head;
}

int main()
{
	message* message_list = new message();
	message_list->inf = "ZDAROVA POCHANY";
	message_list->user = "SAM ZNAESH KTO";
	message_list->next = NULL;
	time_t date = time(NULL);
	tm* datetime = localtime(&date);
	message_list->time = leveling(asctime(datetime));

	WSADATA wsaData; // служебная структура для хранение информации
	// о реализации Windows Sockets
	// старт использования библиотеки сокетов процессом
	// (подгружается Ws2_32.dll)
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// Если произошла ошибка подгрузки библиотеки
	if (result != 0) {
		cerr << "WSAStartup failed: " << result << "\n";
		return result;
	}

	struct addrinfo* addr = NULL; // структура, хранящая информацию
	// об IP-адресе  слущающего сокета

	// Шаблон для инициализации структуры адреса
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET; // AF_INET определяет, что будет
	// использоваться сеть для работы с сокетом
	hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
	hints.ai_protocol = IPPROTO_TCP; // Используем протокол TCP
	hints.ai_flags = AI_PASSIVE; // Сокет будет биндиться на адрес,
	// чтобы принимать входящие соединения

	// Инициализируем структуру, хранящую адрес сокета - addr
	// Наш HTTP-сервер будет висеть на 8000-м порту локалхоста
	result = getaddrinfo("192.168.1.5", "80", &hints, &addr);

	// Если инициализация структуры адреса завершилась с ошибкой,
	// выведем сообщением об этом и завершим выполнение программы
	if (result != 0) {
		cerr << "getaddrinfo failed: " << result << "\n";
		WSACleanup(); // выгрузка библиотеки Ws2_32.dll
		return 1;
	}

	// Создание сокета
	int listen_socket = socket(addr->ai_family, addr->ai_socktype,
		addr->ai_protocol);
	// Если создание сокета завершилось с ошибкой, выводим сообщение,
	// освобождаем память, выделенную под структуру addr,
	// выгружаем dll-библиотеку и закрываем программу
	if (listen_socket == INVALID_SOCKET) {
		cerr << "Error at socket: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		WSACleanup();
		return 1;
	}

	// Привязываем сокет к IP-адресу
	result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);

	// Если привязать адрес к сокету не удалось, то выводим сообщение
	// об ошибке, освобождаем память, выделенную под структуру addr.
	// и закрываем открытый сокет.
	// Выгружаем DLL-библиотеку из памяти и закрываем программу.
	if (result == SOCKET_ERROR) {
		cerr << "bind failed with error: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	// Инициализируем слушающий сокет
	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "listen failed with error: " << WSAGetLastError() << "\n";
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}


	const int max_client_buffer_size = 1024;
	char buf[max_client_buffer_size];
	int client_socket = INVALID_SOCKET;


	ifstream fl1("G:\\ajax1.html");
	string request_body1 = "";
	char c;
	c = fl1.get();
	while (c != EOF)
	{
		request_body1 += c;
		//cout1 << c;
		c = fl1.get();
	}

	for (;;) {
		// Принимаем входящие соединения
		client_socket = accept(listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			cerr << "accept failed: " << WSAGetLastError() << "\n";
			closesocket(listen_socket);
			WSACleanup();
			return 1;
		}

		result = recv(client_socket, buf, max_client_buffer_size, 0);

		std::stringstream response; // сюда будет записываться ответ клиенту
		std::stringstream response_body; // тело ответа

		if (result == SOCKET_ERROR) {
			// ошибка получения данных
			cerr << "recv failed: " << result << "\n";
			closesocket(client_socket);
		}
		else if (result == 0) {
			// соединение закрыто клиентом
			cerr << "connection closed...\n";
		}
		else if (result > 0) {
			// Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
			// В буфере запроса.
			buf[result] = '\0';

			//Принятие и формирование сообщения
			string request = buf;
			int username_index = request.find("username");
			int message_index = request.find("message");
			string username_request;
			string message_request;
			if (username_index != -1 && message_index != -1) {
				username_request = replace(request.substr(username_index + 9, message_index - username_index - 10), '+', ' ');
				message_request = replace(request.substr(message_index + 8), '+', ' ');
				if (message_request == "%2Fclear")
					message_list = clear(message_list);
				else
					if (message_request != "") {
						message_list = add_message(message_list, message_request, username_request);
					}
			}

			// Данные успешно получены
			// формируем тело ответа (HTML)

			if (request.substr(0, 9) == "GET /ajax")
				response_body << print_list_browser(message_list);
			else
				response_body
				<< request_body1
				<< "<form action=\"/\" method=\"POST\">\n"
				<< "<input type=\"text\" name=\"username\" value = \"" + username_request + "\"><br>\n"
				<< "<input type=\"text\" name=\"message\"><br>\n"
				<< "<input type=\"submit\" value=\"Send message\"><br>\n"
				<< "</form><br>\n"
				<< "<div id=\"chat\"><br>\n"
				<< print_list_browser(message_list)
				<< "</div><br>\n"
				<< "</body>\n"
				<< "</html>";

			// Формируем весь ответ вместе с заголовками
			response
				<< "HTTP/1.1 200 OK\r\n"
				<< "Version: HTTP/1.1\r\n"
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "Content-Length: " << response_body.str().length()
				<< "\r\n\r\n"
				<< response_body.str();

			// Отправляем ответ клиенту с помощью функции send
			result = send(client_socket, response.str().c_str(),
				response.str().length(), 0);

			if (result == SOCKET_ERROR) {
				// произошла ошибка при отправле данных
				cerr << "send failed: " << WSAGetLastError() << "\n";
			}
			// Закрываем соединение к клиентом
			closesocket(client_socket);
		}
		system("cls");
		print_list_console(message_list);
	}

	// Убираем за собой
	closesocket(listen_socket);
	freeaddrinfo(addr);
	WSACleanup();
	return 0;
}
