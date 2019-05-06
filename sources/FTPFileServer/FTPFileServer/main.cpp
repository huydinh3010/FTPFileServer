#include <iostream>
#include <WinSock2.h>

#define NAME_LENGTH 50
#define PATH_MAX 100

using namespace std;

int cnum = 0;

struct User {
	char username[NAME_LENGTH];
	char password[NAME_LENGTH];
	boolean perm;
	char path[PATH_MAX];
};

DWORD WINAPI clientThread(LPVOID p) {
	SOCKET c = (SOCKET)p;
	// gửi message
	char username[NAME_LENGTH] = "";
	char password[NAME_LENGTH] = "";
	char message[] = "Chao may.\n";
	send(c, message, strlen(message), 0);
	
	// login
	boolean login = false;

	while (true) {

		char rbuffer[1024];
		memset(rbuffer, 0, 1024);
		recv(c, rbuffer, 1023, 0);

		if (strncmp(rbuffer, "USER", 4) == 0) {

			if (login) login = false;

			if (strchr(rbuffer, ' ') == NULL || strlen(strchr(rbuffer, ' ') + 1) == 1) {
				// nếu nhận được username trống hoặc username = '\n'
				// gửi mã 501: lỗi cú pháp ở tham số lệnh
				char sbuf[] = "501 Syntax error!\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(username) != 0) {
				strcpy(username, strchr(rbuffer, ' ') + 1); // lấy username: USER<SP><username>\n
				username[strlen(username) - 1] = '\0'; // xóa \n
				char sbuf[] = "331 Password required for ";
				strcat(sbuf, username);
				strcat(sbuf, "\n");
				send(c, sbuf, strlen(sbuf), 0);
			}
			
		}
		else if (strncmp(rbuffer, "PASS", 4) == 0) {

			



		}
		else {
			char sbuf[] = "500 Systax error, command unrecognized.\n";
			send(c, sbuf, strlen(sbuf), 0);
		}

	}

	
	
	return 0;

}

int main() {
	WSADATA data;
	int code;
	code = WSAStartup(MAKEWORD(2, 2), &data);
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(21);
	saddr.sin_addr.s_addr = 0;
	code = bind(s, (sockaddr*)&saddr, sizeof(saddr));
	
	code = listen(s, 10);
	while (true) {
		SOCKADDR_IN caddr;
		int clen = sizeof(caddr);
		SOCKET c = accept(s, (sockaddr*)&caddr, &clen);
		cout << "Mot client ket noi den" << endl;
		cnum++;
		CreateThread(NULL, 0, clientThread, (LPVOID)c, 0, NULL);
	}


	return 0;
}