#include <iostream>
#include <WinSock2.h>

using namespace std;
int main() {
	WSADATA data;
	int code;
	code = WSAStartup(MAKEWORD(2, 2), &data);
	SOCKET c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(21);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	code = connect(c, (sockaddr*)&saddr, sizeof(saddr));

	char rbuf[1024];
	memset(rbuf, 0, 1024);
	recv(c, rbuf, 1023, 0);
	cout << rbuf << endl;
	while (true) {
		char sbuf[1024];
		cin.getline(sbuf, 256);
		strcat(sbuf, "\n");
		send(c, sbuf, strlen(sbuf), 0);

		char rbuf[1024];
		memset(rbuf, 0, 1024);
		recv(c, rbuf, 1023, 0);
		cout << rbuf << endl;

	}




	return 0;
}