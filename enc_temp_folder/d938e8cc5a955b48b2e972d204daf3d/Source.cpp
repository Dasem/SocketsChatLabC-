#include <iostream>
#include <sstream>
#include <string>
#include <ctime>

// ��� ���������� ������ freeaddrinfo � MinGW
// ���������: http://stackoverflow.com/a/20306451
#define _WIN32_WINNT 0x501

#include <WinSock2.h>
#include <WS2tcpip.h>

// ����������, ����� �������� ����������� � DLL-�����������
// ��� ������ � �������
#pragma comment(lib, "Ws2_32.lib")

using std::cerr;
using namespace std;

struct message {
	string inf;
	string user;
	time_t time;
	message* next;
};

message* add_message(message* head, string info, string user);
message* clear(message* head);
void print_list_console(message* head);

void print_list_console(message* head) {
	
	for (message* iterator = head; iterator != NULL; iterator = iterator->next) {
		cout << iterator->time <<" "<<iterator->user << " --> " << iterator->inf << "...\n";
	}
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
	message* new_message = (message*)malloc(sizeof(message));
	new_message->inf = info;
	new_message->user = user;
	new_message->next = head;
	new_message->time = time(0);
	head = new_message;
	return head;
}

int main()
{
	WSADATA wsaData; // ��������� ��������� ��� �������� ����������
	// � ���������� Windows Sockets
	// ����� ������������� ���������� ������� ���������
	// (������������ Ws2_32.dll)
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// ���� ��������� ������ ��������� ����������
	if (result != 0) {
		cerr << "WSAStartup failed: " << result << "\n";
		return result;
	}

	struct addrinfo* addr = NULL; // ���������, �������� ����������
	// �� IP-������  ���������� ������

	// ������ ��� ������������� ��������� ������
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET; // AF_INET ����������, ��� �����
	// �������������� ���� ��� ������ � �������
	hints.ai_socktype = SOCK_STREAM; // ������ ��������� ��� ������
	hints.ai_protocol = IPPROTO_TCP; // ���������� �������� TCP
	hints.ai_flags = AI_PASSIVE; // ����� ����� ��������� �� �����,
	// ����� ��������� �������� ����������

	// �������������� ���������, �������� ����� ������ - addr
	// ��� HTTP-������ ����� ������ �� 8000-� ����� ����������
	result = getaddrinfo("127.0.0.1", "8000", &hints, &addr);

	// ���� ������������� ��������� ������ ����������� � �������,
	// ������� ���������� �� ���� � �������� ���������� ���������
	if (result != 0) {
		cerr << "getaddrinfo failed: " << result << "\n";
		WSACleanup(); // �������� ���������� Ws2_32.dll
		return 1;
	}

	// �������� ������
	int listen_socket = socket(addr->ai_family, addr->ai_socktype,
		addr->ai_protocol);
	// ���� �������� ������ ����������� � �������, ������� ���������,
	// ����������� ������, ���������� ��� ��������� addr,
	// ��������� dll-���������� � ��������� ���������
	if (listen_socket == INVALID_SOCKET) {
		cerr << "Error at socket: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		WSACleanup();
		return 1;
	}

	// ����������� ����� � IP-������
	result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);

	// ���� ��������� ����� � ������ �� �������, �� ������� ���������
	// �� ������, ����������� ������, ���������� ��� ��������� addr.
	// � ��������� �������� �����.
	// ��������� DLL-���������� �� ������ � ��������� ���������.
	if (result == SOCKET_ERROR) {
		cerr << "bind failed with error: " << WSAGetLastError() << "\n";
		freeaddrinfo(addr);
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	// �������������� ��������� �����
	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "listen failed with error: " << WSAGetLastError() << "\n";
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}


	const int max_client_buffer_size = 1024;
	char buf[max_client_buffer_size];
	int client_socket = INVALID_SOCKET;

	for (;;) {
		// ��������� �������� ����������
		client_socket = accept(listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			cerr << "accept failed: " << WSAGetLastError() << "\n";
			closesocket(listen_socket);
			WSACleanup();
			return 1;
		}

		result = recv(client_socket, buf, max_client_buffer_size, 0);

		std::stringstream response; // ���� ����� ������������ ����� �������
		std::stringstream response_body; // ���� ������

		if (result == SOCKET_ERROR) {
			// ������ ��������� ������
			cerr << "recv failed: " << result << "\n";
			closesocket(client_socket);
		}
		else if (result == 0) {
			// ���������� ������� ��������
			cerr << "connection closed...\n";
		}
		else if (result > 0) {
			// �� ����� ����������� ������ ���������� ������, ������� ������ ����� ����� ������
			// � ������ �������.
			buf[result] = '\0';

			// ������ ������� ��������
			// ��������� ���� ������ (HTML)
			response_body
				<< "<form action=\"/\" method=\"POST\">"
				<< "<input type=\"text\" name=\"username\"><br>"
				<< "<input type=\"text\" name=\"message\"><br>"
				<< "<input type=\"submit\" value=\"Send message\"><br>"
				<< "</form><br>"
				<< buf;

			// ��������� ���� ����� ������ � �����������
			response << "HTTP/1.1 200 OK\r\n"
				<< "Version: HTTP/1.1\r\n"
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "Content-Length: " << response_body.str().length()
				<< "\r\n\r\n"
				<< response_body.str();

			// ���������� ����� ������� � ������� ������� send
			result = send(client_socket, response.str().c_str(),
				response.str().length(), 0);

			if (result == SOCKET_ERROR) {
				// ��������� ������ ��� �������� ������
				cerr << "send failed: " << WSAGetLastError() << "\n";
			}
			// ��������� ���������� � ��������
			closesocket(client_socket);
		}
	}

	// ������� �� �����
	closesocket(listen_socket);
	freeaddrinfo(addr);
	WSACleanup();
	return 0;
}
