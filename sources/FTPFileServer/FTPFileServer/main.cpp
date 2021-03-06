﻿#include <iostream>
#include <WinSock2.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <regex>
#include <string>

#define NAME_LENGTH 50
#define PATH_LENGTH_MAX 100
#define IP_SERVER "127.0.0.1"

using namespace std;

int cnum = 0;

struct Path {
	int dperm, fperm;
	char virtual_path[PATH_LENGTH_MAX] = "";
	char path[PATH_LENGTH_MAX] = "";
};

struct User {
	char username[NAME_LENGTH] = "";
	char password[NAME_LENGTH] = "";
	vector<Path> paths;
};

vector<User *> users;	// danh sach users

void readConfigFile() {	// doc file config va luu vao list users
	ifstream fileInput("config.txt");
	if (fileInput.fail()) {
		cout << "Loi khi doc file config.txt" << endl;
		system("pause");
		exit(-1);
	}
	while (!fileInput.eof()) {
		User * user = new User;
		char tmp[255] = "";
		fileInput.getline(tmp, 255);
		
		int n;
		sscanf(tmp, "%s%s%d", user->username, user->password, &n);
		for (int i = 0; i < n; i++) {// doc cac duong dan cua user
			memset(tmp, 0, 255);
			fileInput.getline(tmp, 255);
			Path path;	
			// dinh dang <path><\t><virtualpath><\t><fperm><sp><dperm>
			char * firstTab = strchr(tmp, '\t');
			char * secondTab = strchr(firstTab + 1, '\t');
			strncpy(path.path, tmp, firstTab - tmp);	// lay path
			strncpy(path.virtual_path, firstTab + 1, secondTab - firstTab - 1); // virtual path
			sscanf(secondTab + 1, "%d%d", &path.fperm, &path.dperm); // fperm va dperm
			user->paths.push_back(path);
		}
		users.push_back(user);
	}
	fileInput.close();
}

void clear() { // giai phong bo nho
	for (int i = 0; i < users.size(); i++) {
		delete(users.at(i));
	}
}

void showUsersInfo() {
	for (int i = 0; i < users.size(); i++) {
		cout << "username: " << users.at(i)->username << endl;
		cout << "password: " << users.at(i)->password << endl;
		for (int j = 0; j < users.at(i)->paths.size(); j++) {
			cout << "path: " << users.at(i)->paths.at(j).path << " as virtual path: " << users.at(i)->paths.at(j).virtual_path << ". fperm: " << users.at(i)->paths.at(j).fperm << ". dperm: " << users.at(i)->paths.at(j).dperm << endl;
		}
	}
}

DWORD WINAPI clientThread(LPVOID p) {
	SOCKET c = (SOCKET)p;
	User * user;	// tro den cau truc user cua client nay
	
	char username[NAME_LENGTH] = "";
	char password[NAME_LENGTH] = "";

	// gửi message
	char message[] = "Chao may.\n";
	send(c, message, strlen(message), 0);
	bool trans_mode = 0; // 0: chưa set, 1: active mode, 2: passive mode
	SOCKET sdata = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN addr_data;
	addr_data.sin_family = AF_INET;

	// login
	bool login = false;

	while (true) {
		// nhan cau lenh tu client
		char rbuf[1024];
		memset(rbuf, 0, 1024);
		recv(c, rbuf, 1023, 0);



		// xu ly cau lenh
		if (strncmp(rbuf, "USER", 4) == 0) {	// lenh USER<SP><username><\n>
			if (login) login = false;	// phien dang nhap moi
			if (strchr(rbuf, ' ') == NULL || strlen(strchr(rbuf, ' ') + 1) == 1) {
				// nếu nhận được username trống hoặc username = '\n'
				// gửi mã 501: lỗi cú pháp ở tham số lệnh
				char sbuf[] = "501 Syntax error!\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				strcpy(username, strchr(rbuf, ' ') + 1); // lấy username: USER<SP><username>\n
				username[strlen(username) - 1] = '\0'; // xóa \n
				char sbuf[] = "331 Password required for ";
				strcat(sbuf, username);
				strcat(sbuf, "\n");
				send(c, sbuf, strlen(sbuf), 0);
			}
		}
		else if (strncmp(rbuf, "PASS", 4) == 0) {	// lenh PASS<sp><password><\n>
			
			if(!login){ // neu chua login thi kiem tra tai khoan mat khau 
				if (strchr(rbuf, ' ') == NULL || strlen(strchr(rbuf, ' ') + 1) == 1) {
					//bo trong pass thi tim xem co phai la anonymous khong
					for (int i = 0; i < users.size(); i++) {
						if (strcmp(username, users.at(i)->username) == 0 && strcmp("null", users.at(i)->password) == 0) {
							// pass = null tuong ung moi pass
							char sbuf[] = "230 Logged on\n";
							send(c, sbuf, strlen(sbuf), 0);
							login = true;
							user = users.at(i);
							break;
						}
					}
				}
				else {
					strcpy(password, strchr(rbuf, ' ') + 1); // lay password;
					password[strlen(password) - 1] = '\0'; // xoa \n
					// tim trong danh sach user xem co trung username va password khong?
					for (int i = 0; i < users.size(); i++) {
						// password = null tuong ung voi moi pass
						if (strcmp(username, users.at(i)->username) == 0 && (strcmp(password, users.at(i)->password) == 0 || strcmp("null", users.at(i)->password) == 0)) {
							// neu tim thay thi dang nhap thanh cong
							char sbuf[] = "230 Logged on\n";
							send(c, sbuf, strlen(sbuf), 0);
							login = true;
							user = users.at(i);
							break;
						}
					}
				}
				if (!login) { // sau qua trinh kiem tra van khong login duoc
					// gui ma loi tai khoan hoac mat khau khong chinh xac
					char sbuf[] = "530 User or Password incorrect!\n";
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
			else { // dang login ma gui PASS
				// gui ma 503: dang login ma gui command PASS, thi bao loi cu phap
				char sbuf[] = "503 Bad sequence of command\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
		}
		else if (strncmp(rbuf, "PASV", 4) == 0) {
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				closesocket(sdata);
				sdata = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
				while (true) {
					// lưu cấu trúc địa chỉ
					addr_data.sin_addr.s_addr = 0;
					addr_data.sin_port = htons(rand() % (65535 - 1024 + 1) + 1024);	// sinh port ngẫu nhiên trong khoảng từ 1024 -> 65535
					if (!bind(sdata, (sockaddr*)&addr_data, sizeof(addr_data))) break;	// nếu bind thành công thì thoát, tránh trường hợp port đã được sử dụng bởi tiến trình khác
				}
				trans_mode = 2;
				// tạo câu lệnh passive
				char sbuf[] = "227 Entering Passive Mode (";
				char ip[] = IP_SERVER;
				replace(ip, ip + strlen(ip) + 1, '.', ','); // chuyển dấu . thành ,
				strcat(sbuf, ip);
				strcat(sbuf, ",");
				int p1 = ntohs(addr_data.sin_port) / 256;
				int p2 = ntohs(addr_data.sin_port) % 256;
				char port[10] = "";
				sprintf(port, "%d,%d", p1, p2);
				strcat(sbuf, port);
				strcat(sbuf, ")\n");
				send(c, sbuf, strlen(sbuf), 0); // gửi câu lệnh
			}
		}
		else if (strncmp(rbuf, "PORT", 4) == 0) { // lệnh PORT (ip,port) "PORT 122,11,12,5,45,6\n"
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char * first_sp = strchr(rbuf, ' ');
				if (first_sp == NULL || (first_sp - rbuf) != 4) {
					char sbuf[] = "501 Syntax error!\n";
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					char addr[30];
					sscanf(first_sp, "%s", addr);
					bool valid = true;
					char * index[5];
					// kiểm tra xem có đủ 5 dấu ',' không
					index[0] = strchr(addr, ',');
					for (int i = 1; i < 5; i++) {
						index[i] = strchr(index[i - 1] + 1, ',');
						if (index[i] == NULL) {
							valid = false;
							break;
						}
						//*(index[i]) = '\0';
					}
					if (!valid) { // lỗi cú pháp
						char sbuf[] = "501 Syntax error!\n";
						send(c, sbuf, strlen(sbuf), 0);
					}
					else {
						u_char ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0, p1 = 0, p2 = 0;
						// đọc giá trị từ xâu
						sscanf(addr, "%d", &ip1);
						sscanf(index[0] + 1, "%d", &ip2);
						sscanf(index[1] + 1, "%d", &ip3);
						sscanf(index[2] + 1, "%d", &ip4);
						sscanf(index[3] + 1, "%d", &p1);
						sscanf(index[4] + 1, "%d", &p2);
						// kiểm tra giá trị ip có hợp lệ không
						if (ip1 < 0 || ip1 > 255 || ip2 < 0 || ip2 > 255 || ip3 < 0 || ip3 > 255 || ip4 < 0 || ip4 > 255 || p1 < 0 || p1 > 255 || p2 < 0 || p2 > 255) {
							valid = false;
							char sbuf[] = "501 Syntax error!\n";
							send(c, sbuf, strlen(sbuf), 0);
						}
						else {
							// lưu vào cấu trúc địa chỉ
							trans_mode = 1;
							addr_data.sin_addr.S_un.S_un_b.s_b1 = ip1;
							addr_data.sin_addr.S_un.S_un_b.s_b2 = ip2;
							addr_data.sin_addr.S_un.S_un_b.s_b3 = ip3;
							addr_data.sin_addr.S_un.S_un_b.s_b4 = ip4;
							addr_data.sin_port = htons(p1 * 256 + p2);
							/*cout << ip1 << "." << ip2 << "." << ip3 << "." << ip4 << "." << p1 << "." << p2 << endl;*/
							/*cout << addr_data.sin_addr.s_addr << endl;*/
							char sbuf[] = "200 Port command successful!\n";
							send(c, sbuf, strlen(sbuf), 0);
						}
					}
				}
			}
		}


		else {	// lenh khong duoc xu ly -> loi cu phap
			char sbuf[] = "500 Systax error, command unrecognized.\n";
			send(c, sbuf, strlen(sbuf), 0);
		}

	}

	
	
	return 0;

}

int main() {
	// khoi tao wsadata
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
		cout << "Loi khi khoi tao WSA" << endl;
		system("pause");
		return 1;
	}
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(21);
	saddr.sin_addr.s_addr = 0;
	
	if (bind(s, (sockaddr*)&saddr, sizeof(saddr)) != 0) {
		cout << "Loi khi bind socket" << endl;
		system("pause");
		return 1;
	}
	
	if (listen(s, 10) != 0) {
		cout << "Loi khi listen" << endl;
		system("pause");
		return 1;
	}

	readConfigFile(); // doc file config

	while (true) {
		SOCKADDR_IN caddr;
		int clen = sizeof(caddr);
		SOCKET c = accept(s, (sockaddr*)&caddr, &clen);
		cout << "Mot client ket noi den" << endl;
		cnum++;
		CreateThread(NULL, 0, clientThread, (LPVOID)c, 0, NULL);
	}

	clear();
	system("pause");
	return 0;
}