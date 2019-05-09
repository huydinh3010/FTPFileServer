#include <iostream>
#include <WinSock2.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <regex>
#include <string>

#define NAME_LENGTH 50
#define PATH_LENGTH_MAX 200
#define IP_SERVER "127.0.0.1"

using namespace std;

int cnum = 0;

struct User {
	char username[NAME_LENGTH] = "";
	char password[NAME_LENGTH] = "";
	char h_path[PATH_LENGTH_MAX] = "";
	int dperm, fperm;
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
		sscanf(tmp, "%s%s", user->username, user->password);
		char * secondSpace = strchr(strchr(tmp, ' ') + 1, ' ');
		char * firstTab = strchr(tmp, '\t');
		strncpy(user->h_path, secondSpace + 1, firstTab - secondSpace - 1);	// lay path
		sscanf(firstTab, "%d%d", &user->fperm, &user->dperm); // fperm va dperm
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
		cout << "p_path: " << users.at(i)->h_path << endl;
		cout << "fperm: " << users.at(i)->fperm << endl;
		cout << "dperm: " << users.at(i)->dperm << endl;
 	}
}

void strtrim(char * str) { // loại bỏ khoảng trống ở đầu và cuối chuỗi
	while (str[strlen(str) - 1] == ' ') str[strlen(str) - 1] = '\0';
	//char tmp[1024] = "";
	int i = 0;
	while (str[i] == ' ') i++;
	strcpy(str, str + i); // có thể sai ở đây?
}

void preprocessPathname(char * path) {
	if (strlen(path) == 0) return;
	// loại bỏ dấu / ở cuối
	while (path[strlen(path) - 1] == ' ' || path[strlen(path) - 1] == '/') path[strlen(path) - 1] = '\0';
	//if (strlen(path) == 0) { // thư mục gốc
	//	strcpy(path, "/");
	//	return;
	//}
	char tmp[1024] = "";
	int i = 1;
	tmp[0] = path[0];
	// loại bỏ các / gần nhau
	for (int j = 1; j < strlen(path); j++) { 
		if (path[j] == '/' && tmp[i - 1] == '/') continue;
		tmp[i++] = path[j];
	}
	tmp[i] = '\0';
	strcpy(path, tmp);
}

bool splitRequestCommand(const char * requestCmd, char * cmd, char * args) { // tách yêu cầu thành mã và tham số
	char tmp[1024] = "";
	strcpy(tmp, requestCmd);
	while (tmp[strlen(tmp) - 1] == '\n' || tmp[strlen(tmp) - 1] == '\r') tmp[strlen(tmp) - 1] = '\0'; // bỏ qua \r\n.
	char * firstSpace = strchr(tmp, ' '); // tìm sp đầu tiên
	if (firstSpace == NULL) {
		if (strlen(tmp) > 4) return false; // mã command tối đa 4 kí tự
		strcpy(cmd, tmp);
		strupr(cmd);
		strcpy(args, "");
		return true;
	}
	else if (firstSpace - requestCmd > 4) { // mã command tối đa 4 kí tự
		return false;
	}
	else {
		strcpy(args, firstSpace + 1);
		*(firstSpace) = '\0';
		strtrim(args);
		if (strlen(tmp) > 4) return false;
		strcpy(cmd, tmp);
		strupr(cmd);
		return true;
	}
}

int findFile(const char * fullpath) {
	// trả về 0 nếu không tìm thấy, trả về 1 nếu là thư mục, trả về 2 nếu là file, trả về 3 nếu tìm thấy nhiều kết quả
	WIN32_FIND_DATAA FDATA;

	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFileA(fullpath, &FDATA);
	if (hFind == INVALID_HANDLE_VALUE) return 0; // không tìm thấy
	if (FindNextFileA(hFind, &FDATA) == 0) {
		if (FDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return 1; // tìm được 1 thư mục
		else return 2; // tìm được 1 file
	}
	else return 3; // tìm được nhiều hơn 1 file/thư mục
}

bool createListCmdData(const char * fullpath,User * user, char * data) { // tao data cho lenh list
	int type = findFile(fullpath);
	if (type == 0) return false; // không tìm thấy file / thư mục
	else if (type == 1) { // là thư mục
		char tmp[2 * PATH_LENGTH_MAX] = "";
		sprintf(tmp, "%s/*.*", fullpath); // timf kiem tat ca file+thu muc trong thu muc
		WIN32_FIND_DATAA FDATA;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		hFind = FindFirstFileA(tmp, &FDATA);
		//if (hFind == INVALID_HANDLE_VALUE) return true; // khong tim duoc file dau tien, data rỗng, nếu là thư mục chắc chắn tìm được hai thư mục . và ..
		char metadata[100] = "";
		sprintf(metadata, "\t%d\t%s\t%s\t", 1, user->username, user->username);
		do {
			if (FDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				// la thu muc khác .. và .
				if (strcmp(FDATA.cFileName, ".") && strcmp(FDATA.cFileName, "..")) {
					char permission[11] = "d";
					if (user->dperm & 4)  strcat_s(permission, "r");
					else strcat_s(permission, "-");
					if (user->dperm & 2)  strcat_s(permission, "w");
					else strcat_s(permission, "-");
					if (user->dperm & 1)  strcat_s(permission, "x");
					else strcat_s(permission, "-");
					strcat_s(permission, "r-xr-x");
					char line[1024] = "";
					SYSTEMTIME stLocal;
					FileTimeToSystemTime(&FDATA.ftCreationTime, &stLocal);
					sprintf(line, "%s%s%d\t%02d %02d %04d\t%s\n", permission, metadata, FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow, stLocal.wDay, stLocal.wMonth, stLocal.wYear, FDATA.cFileName);
					strcat(data, line);
				}
			}
			else {
				// khong phai thu muc, coi nhu la file
				char permission[11] = "-";
				if (user->fperm & 4)  strcat_s(permission, "r");
				else strcat_s(permission, "-");
				if (user->fperm & 2)  strcat_s(permission, "w");
				else strcat_s(permission, "-");
				if (user->fperm & 1)  strcat_s(permission, "x");
				else strcat_s(permission, "-");
				strcat_s(permission, "r--r--");
				char line[1024] = "";
				SYSTEMTIME stLocal;
				FileTimeToSystemTime(&FDATA.ftCreationTime, &stLocal);
				sprintf(line, "%s%s%d\t%02d %02d %04d\t%s\n", permission, metadata, FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow, stLocal.wDay, stLocal.wMonth, stLocal.wYear, FDATA.cFileName);
				strcat(data, line);
			}
		} while (FindNextFileA(hFind, &FDATA) != 0);
		return true;
	}
	else if (type == 2 || type == 3) { // là file hoặc nhiều file
		WIN32_FIND_DATAA FDATA;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		hFind = FindFirstFileA(fullpath, &FDATA);
		//if (hFind == INVALID_HANDLE_VALUE) return false; // khong tim duoc file
		char metadata[100] = "";
		sprintf(metadata, "\t%d\t%s\t%s\t", 1, user->username, user->username);
		do {
			if (FDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				// la thu muc khác .. và .
				if (strcmp(FDATA.cFileName, ".") && strcmp(FDATA.cFileName, "..")) {
					char permission[11] = "d";
					if (user->dperm & 4)  strcat_s(permission, "r");
					else strcat_s(permission, "-");
					if (user->dperm & 2)  strcat_s(permission, "w");
					else strcat_s(permission, "-");
					if (user->dperm & 1)  strcat_s(permission, "x");
					else strcat_s(permission, "-");
					strcat_s(permission, "r-xr-x");
					char line[1024] = "";
					SYSTEMTIME stLocal;
					FileTimeToSystemTime(&FDATA.ftCreationTime, &stLocal);
					sprintf(line, "%s%s%d\t%02d %02d %04d\t%s\n", permission, metadata, FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow, stLocal.wDay, stLocal.wMonth, stLocal.wYear, FDATA.cFileName);
					strcat(data, line);
				}
			}
			else {
				// khong phai thu muc, coi nhu la file
				char permission[11] = "-";
				if (user->fperm & 4)  strcat_s(permission, "r");
				else strcat_s(permission, "-");
				if (user->fperm & 2)  strcat_s(permission, "w");
				else strcat_s(permission, "-");
				if (user->fperm & 1)  strcat_s(permission, "x");
				else strcat_s(permission, "-");
				strcat_s(permission, "r--r--");
				char line[1024] = "";
				SYSTEMTIME stLocal;
				FileTimeToSystemTime(&FDATA.ftCreationTime, &stLocal);
				sprintf(line, "%s%s%d\t%02d %02d %04d\t%s\n", permission, metadata, FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow, stLocal.wDay, stLocal.wMonth, stLocal.wYear, FDATA.cFileName);
				strcat(data, line);
			}
		} while (FindNextFileA(hFind, &FDATA) != 0);
		return true;
	}
}

DWORD WINAPI clientThread(LPVOID p) {
	SOCKET c = (SOCKET)p;
	// lấy thông tin sockaddr_in của socket c
	SOCKADDR_IN caddr;
	int clen = sizeof(caddr);
	getsockname(c, (sockaddr*)&caddr, &clen);
	//cout << (int)caddr.sin_addr.S_un.S_un_b.s_b1 << "." << (int)caddr.sin_addr.S_un.S_un_b.s_b2 << "." << (int)caddr.sin_addr.S_un.S_un_b.s_b3 << "." << (int)caddr.sin_addr.S_un.S_un_b.s_b4 << endl;

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
	char cwd[PATH_LENGTH_MAX] = "/";

	while (true) {
		// nhan cau lenh tu client
		char rbuf[1024];
		memset(rbuf, 0, 1024);
		recv(c, rbuf, 1023, 0);

		char cmd[5] = "";
		char args[1024] = "";
		if (!splitRequestCommand(rbuf, cmd, args)) {
			char sbuf[] = "500 Systax error, command unrecognized.\n";
			send(c, sbuf, strlen(sbuf), 0);
			continue;
		}

		// xu ly cau lenh
		if (strcmp(cmd, "USER") == 0) {	// lenh USER<SP><username><\n>
			if (strlen(args) > NAME_LENGTH) { // username quá dài
				char sbuf[] = "501 Username too long.\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				if (login) login = false;	// phien dang nhap moi
				if (strlen(args) == 0) {
					// nhận được username trống 
					// gửi mã 501: lỗi cú pháp ở tham số lệnh
					char sbuf[] = "501 Syntax error!\n";
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					strcpy(username, args); // lấy username: USER<SP><username>\n
					char sbuf[] = "331 Password required for ";
					strcat(sbuf, username);
					strcat(sbuf, "\n");
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "PASS") == 0) {	// lenh PASS<sp><password><\n>
			if (strlen(args) > NAME_LENGTH) { // password quá dài
				char sbuf[] = "501 Password too long.\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if(!login){ // neu chua login thi kiem tra tai khoan mat khau 
				if (strlen(args) == 0) {
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
					strcpy(password, args); // lay password;
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
		else if (strcmp(cmd, "PASV") == 0) {
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
		else if (strcmp(cmd, "PORT") == 0) { // lệnh PORT (ip,port) "PORT 122,11,12,5,45,6\n"
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				if (strlen(args) == 0) {
					char sbuf[] = "501 Syntax error!\n";
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					bool valid = true;
					char * index[5];
					// kiểm tra xem có đủ 5 dấu ',' không
					index[0] = strchr(args, ',');
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
						sscanf(args, "%d", &ip1);
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
						//else if (ip1 != caddr.sin_addr.S_un.S_un_b.s_b1 || ip2 != caddr.sin_addr.S_un.S_un_b.s_b2 || ip3 != caddr.sin_addr.S_un.S_un_b.s_b3 || ip4 != caddr.sin_addr.S_un.S_un_b.s_b4) {
						//	// ip từ lệnh port không khớp với ip của client
						//	// có thể không cần kiểm tra thông tin này
						//	// gửi mã 421 không đáp ứng dịch vụ do ip không khớp
						//	char sbuf[] = "421 Rejected command, requested IP address does not match control connection IP.\n";
						//	send(c, sbuf, strlen(sbuf), 0);
						//}
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
							// gửi phản hồi đến client
							char sbuf[] = "200 Port command successful!\n";
							send(c, sbuf, strlen(sbuf), 0);
						}
					}
				}
			}
		}
		else if (strcmp(cmd, "PWD") == 0) { // xử lý lệnh PWD<sp><\r\n>
			char sbuf[1024] = "";
			sprintf(sbuf, "257 \"%s\" is current directory.\r\n", cwd);
			send(c, sbuf, strlen(sbuf), 0);
		}
		else if (strcmp(cmd, "CWD") == 0) { // xử lý lệnh CWD<sp><pathname><\r\n>
			if (strlen(args) == 0) {
				// tham số trống -> đường dẫn như cũ
				char sbuf[1024] = "";
				sprintf(sbuf, "250 missing argument to CWD. \"%s\" is current directory.\r\n", cwd);
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strcmp(args, ".") == 0) { // thư mục hiện tại
				char sbuf[1024] = "";
				sprintf(sbuf, "250 \"%s\" is current directory.\r\n", cwd);
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strcmp(args, "..") == 0) { // thư mục cha
				if (strcmp(cwd, "/") != 0) { // quay lai thư mục cha khi không ở thư mục gốc
					cwd[strlen(cwd) - 1] = '\0';
					char * last = strrchr(cwd, '/');
					if (last - cwd == 0) strcpy(cwd, "/");
					else cwd[last - cwd + 1] = '\0';
				}
				char sbuf[1024] = "";
				sprintf(sbuf, "250 \"%s\" is current directory.\r\n", cwd);
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char path[2 * PATH_LENGTH_MAX] = "";
				if (args[0] == '/') strcpy(path, args); // đường dẫn tuyệt đối
				else sprintf(path, "%s%s", cwd, args); // đường dẫn tương đối
				preprocessPathname(path);
				if (strlen(path) == 0) { // là thư mục gốc
					strcpy(cwd, "/");
					char sbuf[] = "250 successful. \"/\" is current directory.\r\n";
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					// chuyển thành path vật lý
					char fullpath[2 * PATH_LENGTH_MAX] = "";
					sprintf(fullpath, "%s%s", user->h_path, path);
					// tìm trong thư mục xem có chứa file này hay không
					if (findFile(fullpath) == 1) {
						// là thư mục, gửi lệnh ok
						sprintf(cwd, "%s/", path);
						/*strcpy(cwd, path);*/
						char sbuf[1024] = "";
						sprintf(sbuf, "250 successful. \"%s\" is current directory.\r\n", cwd);
						send(c, sbuf, strlen(sbuf), 0);
					}
					else {
						char sbuf[1024] = "";
						sprintf(sbuf, "550 CWD failed. \"%s\": directory not found.\r\n", path);
						send(c, sbuf, strlen(sbuf), 0);
					}
				}
			}
		}
		else if (strcmp(cmd, "CDUP") == 0) { // chuyển về thư mục cha
			if (strcmp(cwd, "/") != 0) { // quay lai thư mục cha khi không ở thư mục gốc
				cwd[strlen(cwd) - 1] = '\0';
				char * last = strrchr(cwd, '/');
				if (last - cwd == 0) strcpy(cwd, "/");
				else cwd[last - cwd + 1] = '\0';
			}
			char sbuf[1024] = "";
			sprintf(sbuf, "200 CDUP successful. \"%s\" is current directory.\r\n", cwd);
			send(c, sbuf, strlen(sbuf), 0);
		}
		else if (strcmp(cmd, "LIST") == 0) { // xử lý lệnh LIST<sp><path><\n>
			if(!login){
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(args) > PATH_LENGTH_MAX) {
				char sbuf[] = "501 Pathname too long.\n";
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (trans_mode == 0) {
				char sbuf[] = "425 Use PASV or PORT command first.\n";
				send(c, sbuf, strlen(sbuf), 0);
			} 
			else{
				char path[2*PATH_LENGTH_MAX] = "";
				if (strlen(args) == 0) strcpy(path, cwd); // path bỏ trống, lấy path trong cwd
				else {
					if (args[0] == '/') strcpy(path, args);// đường dẫn tuyệt đối
					else sprintf(path, "%s%s", cwd, args); // đường dẫn tương đối
				}
				// chưa xử lý kí tự đặc biệt như: cd .., cd ., cd ~ giống trong linux
				
				preprocessPathname(path);
				if (strcmp(path, "/") == 0) path[0] = '\0';
				// ghép thành đường dẫn vật lý
				char fullpath[2 * PATH_LENGTH_MAX] = "";
				sprintf(fullpath, "%s%s", user->h_path, path);

				//replace(fullpath, fullpath + strlen(fullpath) + 1, '/','\\'); // thay thế ký tự / thành '\'
				//// tìm và xử lý trong bộ nhớ
				
				char data[1024] = "";
				if (createListCmdData(fullpath, user, data)) {
					//data[strlen(data) - 1] = '\0'; // xóa \n ở cuối
					// mở kết nối mới và gửi dữ liệu đi
					// gửi mã 150
					char sbuf[1024] = "150 Opening data channel for directory listing of ";
					strcat_s(sbuf, args);
					strcat_s(sbuf, "\n");
					send(c, sbuf, strlen(sbuf), 0);
					if (trans_mode == 1) {
						// active mode
						// đặt cổng kết nối của server là cổng 20, bằng hàm bind
						sdata = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
						SOCKADDR_IN sdata_addr;
						sdata_addr.sin_family = AF_INET;
						sdata_addr.sin_addr.s_addr = 0;
						sdata_addr.sin_port = htons(20);
						bind(sdata, (sockaddr*)&sdata_addr, sizeof(sdata_addr)); // có thể bind lỗi tại đây

						int code = connect(sdata, (sockaddr*)&addr_data, sizeof(addr_data));
						if (code != 0) {
							// không kết nối được
							// gửi code báo lỗi
							char sbuf[] = "425 Can't open data connection.\n";
							send(c, sbuf, strlen(sbuf), 0);
						}
						else {
							// gửi dữ liệu đi
							// gửi code báo thành công
							send(sdata, data, strlen(data), 0);
							closesocket(sdata); // đóng kết nối
							char sbuf[1024] = "226 Successful transfering ";
							strcat_s(sbuf, args);
							strcat_s(sbuf, "\n");
							send(c, sbuf, strlen(sbuf), 0);
						}
					}
					else if (trans_mode == 2) {
						// passive mode
						listen(sdata, 1);
						SOCKADDR_IN caddr_t;
						int clen_t = sizeof(caddr_t);
						SOCKET sd = accept(sdata, (sockaddr*)&caddr_t, &clen_t);
						// chưa set timeout cho hàm accept tránh trường hợp chờ vô hạn
						// sử dụng select để set timeout.
						send(sd, data, strlen(data), 0);
						closesocket(sd);
						char sbuf[1024] = "226 Successful transfering ";
						strcat_s(sbuf, args);
						strcat_s(sbuf, "\n");
						send(c, sbuf, strlen(sbuf), 0);
					}
					trans_mode = 0;
				}
				else {
					// không tìm thấy, gửi thông báo lỗi
					// gửi mã 550 File or Directory not found.
					char sbuf[] = "550 File or Directory not found.\n";
					send(c, sbuf, strlen(sbuf), 0);
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
	//showUsersInfo();

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