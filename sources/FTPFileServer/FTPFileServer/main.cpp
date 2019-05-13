// author: Nguyễn Huy Định + Lã Hồng Anh + Nguyễn Hải Sơn
#include <iostream>
#include <WinSock2.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>

#define NAME_LENGTH 50
#define PATH_LENGTH_MAX 1024
#define IP_SERVER "127.0.0.1"
#define ASCII_DATA_TYPE 1
#define BINARY_DATA_TYPE 2	

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
	int i = 0;
	while (str[i] == ' ') i++;
	strcpy(str, str + i);
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

void processPathname(const char * cwd, const char * home, char * args, char * v_path, char * p_path) {
	// xu ly args de lay ra duong dan.
	if (strlen(args) == 0 || strcmp(args, "-l") == 0) strcpy(v_path, cwd); // path bỏ trống, lấy path trong cwd
	else if (args[0] == '/') strcpy(v_path, args);// đường dẫn tuyệt đối
	else sprintf(v_path, "%s%s", cwd, args); // đường dẫn tương đối dạng folder1/
	// tien xu ly v_path.
	if (strlen(v_path) != 0) {
		while (v_path[strlen(v_path) - 1] == ' ' || v_path[strlen(v_path) - 1] == '/') v_path[strlen(v_path) - 1] = '\0'; // loại bỏ dấu / và sp ở cuối chuỗi
		char tmp[1024] = "";
		int i = 1;
		tmp[0] = v_path[0];
		for (int j = 1; j < strlen(v_path); j++) { // loại bỏ các / gần nhau
			if (v_path[j] == '/' && tmp[i - 1] == '/') continue;
			tmp[i++] = v_path[j];
		}
		tmp[i] = '\0';
		strcpy(v_path, tmp);
	}
	char * sstr;
	strcat(v_path, "/");
	while ((sstr = strstr(v_path, "/./")) != NULL) { // loại bỏ các chuỗi dạng /./ trong v_path
		sstr[1] = '\0';
		strcat(v_path, sstr + 3);
	}
	while ((sstr = strstr(v_path, "/../")) != NULL) { // xử lý các chuỗi dạng /../ trong v_path
		if (sstr == v_path) strcpy(v_path, v_path + 3);
		else {
			sstr[0] = '\0';
			char * last = strrchr(v_path, '/');
			last[1] = '\0';
			strcat(v_path, sstr + 4);
		}
	}
	v_path[strlen(v_path) - 1] = '\0';
	sprintf(p_path, "%s%s", home, v_path);
}

int findFile(const char * fullpath) { // trả về 0 nếu không tìm thấy, trả về 1 nếu là thư mục, trả về 2 nếu là file, trả về 3 nếu tìm thấy nhiều kết quả	
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

int sizeOfFile(const char * fullpath) { // trả về kích thươc file nếu tìm thấy, trả về 0 nếu không tìm thấy
	WIN32_FIND_DATAA FDATA;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFileA(fullpath, &FDATA);
	if (hFind == INVALID_HANDLE_VALUE) return 0;
	int size = FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow;
	if (FindNextFileA(hFind, &FDATA) == 0) {
		if (FDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return 0; // tìm được 1 thư mục
		else return size; // tìm được 1 file
	}
	else return 0; // tìm được nhiều hơn 1 file/thư mục
}

bool createActiveDataConnection(SOCKET * sdata, SOCKADDR_IN cdata_addr) {
	*sdata = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN sdata_addr;
	sdata_addr.sin_family = AF_INET;
	sdata_addr.sin_addr.s_addr = 0;
	sdata_addr.sin_port = htons(20); // đặt cổng kết nối cho server, theo giao thức FTP cổng này là 20
	bind(*sdata, (sockaddr*)&sdata_addr, sizeof(sdata_addr)); // có thể bind lỗi tại đây, nếu lỗi có thể sẽ kết nối bằng cổng khác

	if (connect(*sdata, (sockaddr*)&cdata_addr, sizeof(cdata_addr)) == 0) return true;
	return false;
}

bool createPassiveDataConnection(SOCKET * sdata, SOCKADDR_IN caddr) { // tạo socket kết nối kiểu bị động của kênh dữ liệu
	// trả về true nếu tạo thành công, socket trả về là sdata
	fd_set fdread;
	while (true) {
		FD_ZERO(&fdread);
		FD_SET(*sdata, &fdread);
		SOCKADDR_IN caddr_t;
		int clen_t = sizeof(caddr_t);
		timeval timeout;
		timeout.tv_sec = 60; //time out là 1p
		timeout.tv_usec = 0;
		select(0, &fdread, NULL, NULL, &timeout);
		if (FD_ISSET(*sdata, &fdread)) {
			SOCKET sd = accept(*sdata, (sockaddr*)&caddr_t, &clen_t);
			if (caddr_t.sin_addr.S_un.S_addr == caddr.sin_addr.S_un.S_addr) { // ip kết nối đến trùng với ip client
				*sdata = sd;
				return true;
			}
		}
		else return false; // xảy ra lỗi hoặc timeout
	}
}

bool construcListCmdData(const char * fullpath, User * user, char ** data) { // tao data cho lenh list
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
		sprintf(metadata, " %d %s %s ", 1, user->username, user->username);
		char month[12][4] = { "Jan","Feb","Mar","Apr","May","Jun","Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		int data_size = 1; // kich thuoc cua du lieu
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
					FileTimeToSystemTime(&FDATA.ftLastAccessTime, &stLocal);
					sprintf(line, "%s%s%10d %s %02d %04d %s\r\n", permission, metadata, FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow, month[stLocal.wMonth], stLocal.wDay, stLocal.wYear, FDATA.cFileName);
					data_size += strlen(line);
					*data = (char *)realloc(*data, data_size);
					strcat(*data, line);
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
				FileTimeToSystemTime(&FDATA.ftLastAccessTime, &stLocal);
				sprintf(line, "%s%s%10d %s %02d %04d %s\r\n", permission, metadata, FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow, month[stLocal.wMonth], stLocal.wDay, stLocal.wYear, FDATA.cFileName);
				data_size += strlen(line);
				*data = (char *)realloc(*data, data_size);
				strcat(*data, line);
			}
		} while (FindNextFileA(hFind, &FDATA) != 0);
		return true;
	}
	else if (type == 2 || type == 3) { // là file hoặc nhiều file, tìm theo ký tự đặc biệt vd: LIST *.txt
		WIN32_FIND_DATAA FDATA;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		hFind = FindFirstFileA(fullpath, &FDATA);
		char metadata[100] = "";
		sprintf(metadata, " %5d %s %s ", 1, user->username, user->username);
		char month[12][4] = { "Jan","Feb","Mar","Apr","May","Jun","Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		int data_size = 1; // kich thuoc cua du lieu
		do {
			if (FDATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
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
					FileTimeToSystemTime(&FDATA.ftLastAccessTime, &stLocal);
					sprintf(line, "%s%s%10d %s %02d %04d %s\r\n", permission, metadata, FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow, month[stLocal.wMonth], stLocal.wDay, stLocal.wYear, FDATA.cFileName);
					data_size += strlen(line);
					*data = (char *)realloc(*data, data_size);
					strcat(*data, line);
				}
			}
			else {
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
				FileTimeToSystemTime(&FDATA.ftLastAccessTime, &stLocal);
				sprintf(line, "%s%s%10d %s %02d %04d %s\r\n", permission, metadata, FDATA.nFileSizeHigh * (MAXDWORD + 1) + FDATA.nFileSizeLow, month[stLocal.wMonth], stLocal.wDay, stLocal.wYear, FDATA.cFileName);
				data_size += strlen(line);
				*data = (char *)realloc(*data, data_size);
				strcat(*data, line);
			}
		} while (FindNextFileA(hFind, &FDATA) != 0);
		return true;
	}
}

bool construcNameListCmdData(const char * fullpath, User * user, char ** data) { // tương tự LIST nhưng chỉ lấy danh sách tên
	int type = findFile(fullpath);
	if (type == 0) return false; // không tìm thấy file / thư mục
	else if (type == 1) { // là thư mục
		char tmp[2 * PATH_LENGTH_MAX] = "";
		sprintf(tmp, "%s/*.*", fullpath); // timf kiem tat ca file+thu muc trong thu muc
		WIN32_FIND_DATAA FDATA;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		hFind = FindFirstFileA(tmp, &FDATA);
		int data_size = 1;
		do {
			char line[300] = "";
			sprintf(line, "%s\r\n", FDATA.cFileName);
			data_size += strlen(line);
			*data = (char *)realloc(*data, data_size);
			strcat(*data, line);
		} while (FindNextFileA(hFind, &FDATA) != 0);
		return true;
	}
	else if (type == 2 || type == 3) { // là file hoặc nhiều file
		WIN32_FIND_DATAA FDATA;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		hFind = FindFirstFileA(fullpath, &FDATA);
		int data_size = 1;
		do {
			char line[300] = "";
			sprintf(line, "%s\r\n", FDATA.cFileName);
			data_size += strlen(line);
			*data = (char *)realloc(*data, data_size);
			strcat(*data, line);
		} while (FindNextFileA(hFind, &FDATA) != 0);
		return true;
	}
}

DWORD WINAPI clientThread(LPVOID p) {
	int * client_info = (int *)p;
	SOCKET c = client_info[0];
	SOCKADDR_IN caddr;
	caddr.sin_family = AF_INET;
	caddr.sin_addr.S_un.S_un_b.s_b1 = client_info[1];
	caddr.sin_addr.S_un.S_un_b.s_b2 = client_info[2];
	caddr.sin_addr.S_un.S_un_b.s_b3 = client_info[3];
	caddr.sin_addr.S_un.S_un_b.s_b4 = client_info[4];
	caddr.sin_port = client_info[5];
	cout << "Client info: " << client_info[1] << "." << client_info[2] << "." << client_info[3] << "." << client_info[4] << " " << client_info[5] << endl;
	User * user;	// tro den cau truc user cua client nay
	char username[NAME_LENGTH] = "";
	char password[NAME_LENGTH] = "";
	// gửi message
	char message[] = "220 Simple FTP Server.\r\n";
	cout << "Send to " << c << ": " << message << endl;
	send(c, message, strlen(message), 0);
	int trans_mode = 0; // 0: chưa set, 1: active mode, 2: passive mode
	int data_type = ASCII_DATA_TYPE; // 0: ascii (default), 1: binary
	// socket kết nối kênh dữ liệu
	SOCKET sdata = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN addr_data;
	addr_data.sin_family = AF_INET;
	bool login = false;
	char cwd[PATH_LENGTH_MAX] = "/";
	char fileReadyForRename[PATH_LENGTH_MAX] = "";
	fd_set fdread;
	while (true) {
		FD_ZERO(&fdread);
		FD_SET(c, &fdread);
		timeval timeout;
		timeout.tv_sec = 120; // quá 2 phút -> time out
		timeout.tv_usec = 0;
		select(0, &fdread, NULL, NULL, &timeout);
		char rbuf[1024];
		memset(rbuf, 0, 1024);
		if (FD_ISSET(c, &fdread)) {
			// nhan cau lenh tu client
			if (recv(c, rbuf, 1023, 0) <= 0) break; // client ngắt kết nối
		}
		else {
			// xảy ra lỗi hoặc timeout
			char sbuf[] = "421 Timeout\r\n";
			cout << "Send to " << c << ": " << sbuf << endl;
			send(c, sbuf, strlen(sbuf), 0);
			break;
		}
		cout << "Recv from " << c << ": " << rbuf << endl;
		char cmd[5] = "";
		char args[1024] = "";
		if (!splitRequestCommand(rbuf, cmd, args)) {
			char sbuf[] = "500 Systax error, command unrecognized.\r\n";
			send(c, sbuf, strlen(sbuf), 0);
			continue;
		}
		if (strcmp(cmd, "USER") == 0) {	// lenh USER<SP><username><\n>
			if (strlen(args) > NAME_LENGTH) { // username quá dài
				char sbuf[] = "501 Username too long.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				if (login) login = false;	// phien dang nhap moi
				if (strlen(args) == 0) {
					// nhận được username trống 
					// gửi mã 501: lỗi cú pháp ở tham số lệnh
					char sbuf[] = "501 Syntax error!\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					strcpy(username, args); // lấy username: USER<SP><username>\n
					char sbuf[1024] = "";
					sprintf(sbuf, "331 Password required for %s\r\n", username);
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "PASS") == 0) {	// lenh PASS<sp><password><\n>
			if (strlen(args) > NAME_LENGTH) { // password quá dài
				char sbuf[] = "501 Password too long.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (!login) { // neu chua login thi kiem tra tai khoan mat khau 
				if (strlen(args) == 0) {
					//bo trong pass thi tim xem co phai la anonymous khong
					for (int i = 0; i < users.size(); i++) {
						if (strcmp(username, users.at(i)->username) == 0 && strcmp("null", users.at(i)->password) == 0) {
							// pass = null tuong ung moi pass
							char sbuf[] = "230 Logged on.\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
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
							char sbuf[] = "230 Logged on.\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
							send(c, sbuf, strlen(sbuf), 0);
							login = true;
							user = users.at(i);
							break;
						}
					}
				}
				if (!login) { // sau qua trinh kiem tra van khong login duoc
					// gui ma loi tai khoan hoac mat khau khong chinh xac
					char sbuf[] = "530 User or Password incorrect!.\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
			else { // dang login ma gui PASS
				// gui ma 503: dang login ma gui command PASS, thi bao loi cu phap
				char sbuf[] = "503 Bad sequence of command.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
		}
		else if (strcmp(cmd, "PASV") == 0) {
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				closesocket(sdata);
				sdata = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				while (true) {
					// lưu cấu trúc địa chỉ
					addr_data.sin_addr.s_addr = 0;
					addr_data.sin_port = htons(rand() % (65535 - 1024 + 1) + 1024);	// sinh port ngẫu nhiên trong khoảng từ 1024 -> 65535
					if (!bind(sdata, (sockaddr*)&addr_data, sizeof(addr_data))) { // nếu bind thành công thì thoát, tránh trường hợp port đã được sử dụng bởi tiến trình khác
						listen(sdata, 5);
						break;
					}
				}
				trans_mode = 2;
				// tạo câu lệnh passive
				char sbuf[1024] = "";
				char ip[] = IP_SERVER;
				replace(ip, ip + strlen(ip) + 1, '.', ','); // chuyển dấu . thành ,
				sprintf(sbuf, "227 Entering Passive Mode (%s,%d,%d)\r\n", ip, ntohs(addr_data.sin_port) / 256, ntohs(addr_data.sin_port) % 256);
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0); // gửi câu lệnh
			}
		}
		else if (strcmp(cmd, "PORT") == 0) { // lệnh PORT (ip,port) "PORT 122,11,12,5,45,6\n"
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				if (strlen(args) == 0) {
					char sbuf[] = "501 Syntax error!\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
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
					}
					if (!valid) { // lỗi cú pháp
						char sbuf[] = "501 Syntax error!\r\n";
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
					}
					else {
						int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0, p1 = 0, p2 = 0;
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
							char sbuf[] = "501 Syntax error!\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
							send(c, sbuf, strlen(sbuf), 0);
						}
						else if (ip1 != caddr.sin_addr.S_un.S_un_b.s_b1 || ip2 != caddr.sin_addr.S_un.S_un_b.s_b2 || ip3 != caddr.sin_addr.S_un.S_un_b.s_b3 || ip4 != caddr.sin_addr.S_un.S_un_b.s_b4) {
							// ip từ lệnh port không khớp với ip của client
							// có thể không cần kiểm tra thông tin này
							// gửi mã 421 không đáp ứng dịch vụ do ip không khớp
							char sbuf[] = "421 Rejected command, requested IP address does not match control connection IP.\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
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
							// gửi phản hồi đến client
							char sbuf[] = "200 Port command successful!\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
							send(c, sbuf, strlen(sbuf), 0);
						}
					}
				}
			}
		}
		else if (strcmp(cmd, "PWD") == 0) { // xử lý lệnh PWD<sp><\r\n>
			if (!login) {
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char sbuf[1024] = "";
				sprintf(sbuf, "257 \"%s\" is current directory.\r\n", cwd);
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
		}
		else if (strcmp(cmd, "CWD") == 0) { // xử lý lệnh CWD<sp><pathname><\r\n>
			if (!login) { // chưa login thì không được sử dụng
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (user->dperm & 5 != 5) { // phải có quyền execute và quyền read với thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(args) == 0) { // tham số trống -> đường dẫn như cũ
				char sbuf[1024] = "";
				sprintf(sbuf, "250 missing argument to CWD. \"%s\" is current directory.\r\n", cwd);
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strcmp(args, "/") == 0) { // là thư mục gốc
				strcpy(cwd, "/");
				char sbuf[] = "250 successful. \"/\" is current directory.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				if (findFile(p_path) == 1) { // tìm xem có chứa thư mục này hay không
					// là thư mục, gửi lệnh ok
					sprintf(cwd, "%s/", v_path);
					char sbuf[1024] = "";
					sprintf(sbuf, "250 successful. \"%s\" is current directory.\r\n", cwd);
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					char sbuf[1024] = "";
					sprintf(sbuf, "550 CWD failed. \"%s\": directory not found.\r\n", args);
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}

			}
		}
		else if (strcmp(cmd, "CDUP") == 0) { // chuyển về thư mục cha
			if (!login) { // yêu cầu login trước
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (user->dperm & 5 != 5) { // phải có quyền execute và quyền read với thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strcmp(cwd, "/") != 0) { // quay lai thư mục cha khi không ở thư mục gốc
				cwd[strlen(cwd) - 1] = '\0';
				char * last = strrchr(cwd, '/');
				if (last - cwd == 0) strcpy(cwd, "/");
				else cwd[last - cwd + 1] = '\0';
			}
			char sbuf[1024] = "";
			sprintf(sbuf, "200 CDUP successful. \"%s\" is current directory.\r\n", cwd);
			cout << "Send to " << c << ": " << sbuf << endl;
			send(c, sbuf, strlen(sbuf), 0);
		}
		else if (strcmp(cmd, "LIST") == 0) { // xử lý lệnh LIST<sp><path><\n>
			if (!login) { // yêu cầu login trước
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (!(user->dperm & 4)) { // phải có quyền read với thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (trans_mode == 0) { // phải có lệnh đặt chế độ trước khi truyền dữ liệu
				char sbuf[] = "425 Use PASV or PORT command first.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				char * data = (char *)malloc(1);
				*data = '\0';
				if (construcListCmdData(p_path, user, &data)) {
					// mở kết nối mới và gửi dữ liệu đi
					// gửi mã 150
					char sbuf[1024] = "";
					sprintf(sbuf, "150 Opening data channel for directory listing of %s\r\n", args);
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
					bool b = false;
					if (trans_mode == 1) b = createActiveDataConnection(&sdata, addr_data);
					else if (trans_mode == 2) b = createPassiveDataConnection(&sdata, caddr);
					if (b) {
						send(sdata, data, strlen(data), 0);
						closesocket(sdata); // đóng kết nối
						char sbuf[] = "226 Successful transfering\r\n";
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
					}
					else {
						char sbuf[] = "425 Can't open data connection.\r\n";
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
					}
					trans_mode = 0;
				}
				else {
					// không tìm thấy, gửi thông báo lỗi
					// gửi mã 550 File or Directory not found.
					char sbuf[] = "550 File or Directory not found.\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
				free(data);
			}
		}
		else if (strcmp(cmd, "NLST") == 0) { // name list command
			if (!login) {
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (!(user->dperm & 4)) { // phải có quyền read với thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (trans_mode == 0) {
				char sbuf[] = "425 Use PASV or PORT command first.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				char * data = (char *)malloc(1);
				*data = '\0';
				if (construcNameListCmdData(p_path, user, &data)) {
					char sbuf[1024] = "";
					sprintf(sbuf, "150 Opening data channel for directory listing of %s\r\n", args);
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
					bool b = false;
					if (trans_mode == 1) b = createActiveDataConnection(&sdata, addr_data);
					else if (trans_mode == 2) b = createPassiveDataConnection(&sdata, caddr);
					if (b) {
						send(sdata, data, strlen(data), 0);
						closesocket(sdata); // đóng kết nối
						char sbuf[] = "226 Successful transfering\r\n";
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
					}
					else {
						char sbuf[] = "425 Can't open data connection.\r\n";
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
					}
					trans_mode = 0;
				}
				else {
					// không tìm thấy, gửi thông báo lỗi
					// gửi mã 550 File or Directory not found.
					char sbuf[] = "550 File or Directory not found.\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
				free(data);
			}
		}
		else if (strcmp(cmd, "RETR") == 0) { // tải dữ liệu từ server
			if (!login) { // yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(args) == 0) { // không được bỏ trống tên file
				char sbuf[] = "501 Syntax error\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (!(user->fperm & 4)) { // phải có quyền read với file
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (trans_mode == 0) {
				char sbuf[] = "425 Use PASV or PORT command first.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				if (findFile(p_path) == 2) { //tìm thấy file
					bool canOpenFile = true;
					int sizeOfDataSend = 0;
					FILE * f = fopen(p_path, "rb");
					if (f != NULL) {
						fseek(f, 0, SEEK_END);
						long filesize = ftell(f);
						char * data = (char*)calloc(filesize, 1);
						fseek(f, 0, SEEK_SET);
						fread(data, 1, filesize, f);
						sizeOfDataSend = filesize;
						fclose(f);
						char sbuf[1024] = "150 Opening data channel\r\n";
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
						bool b = false;
						if (trans_mode == 1) b = createActiveDataConnection(&sdata, addr_data);
						else if (trans_mode == 2) b = createPassiveDataConnection(&sdata, caddr);
						if (b) {
							send(sdata, data, sizeOfDataSend, 0);
							closesocket(sdata); // đóng kết nối
							char sbuf[] = "226 Successful transfering\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
							send(c, sbuf, strlen(sbuf), 0);
						}
						else {
							char sbuf[] = "425 Can't open data connection.\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
							send(c, sbuf, strlen(sbuf), 0);
						}
						trans_mode = 0;
						free(data); // giải phóng bộ nhớ
					}
					else {
						char sbuf[] = "550 Cannot open file.\r\n";
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
					}
				}
				else {
					char sbuf[] = "550 File not found.\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "STOR") == 0) { // tải file lên server
			if (!login) { // yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(args) == 0) { // không được bỏ trống tên file
				char sbuf[] = "501 Syntax error\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (!(user->fperm & 2) || !(user->dperm & 2)) { // phải có quyền write với file và thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (trans_mode == 0) {
				char sbuf[] = "425 Use PASV or PORT command first.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				// lấy tên file và đường dẫn tới thư mục chứa file.
				char dpath[2 * PATH_LENGTH_MAX] = "";
				char filename[PATH_LENGTH_MAX] = "";
				strcpy(dpath, p_path);
				char * last = strrchr(dpath, '/');
				strcpy(filename, last + 1);
				*last = '\0';
				if (findFile(dpath) == 1 && findFile(p_path) == 0) { // tìm thấy thư mục nhưng không tìm thấy file (file chưa tồn tại)
					FILE * f = fopen(p_path, "wb");
					if (f != NULL) {
						char sbuf[1024] = "";
						sprintf(sbuf, "150 Opening data channel for file upload to server of \"%s\"\r\n", args);
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
						bool b = false;
						if (trans_mode == 1) b = createActiveDataConnection(&sdata, addr_data);
						else if (trans_mode == 2) b = createPassiveDataConnection(&sdata, caddr);
						if (b) {
							// nhận dữ liệu và ghi vào file
							fd_set fdread;
							do {
								FD_ZERO(&fdread);
								FD_SET(sdata, &fdread);
								timeval timeout;
								timeout.tv_sec = 60; // timeout là 1p
								timeout.tv_usec = 0;
								select(0, &fdread, NULL, NULL, &timeout); 
								if (FD_ISSET(sdata, &fdread)) {
									char dbuf[1024];
									memset(dbuf, 0, sizeof(dbuf));
									int size = recv(sdata, dbuf, sizeof(dbuf) - 1, 0);
									if (size > 0) fwrite(dbuf, 1, size, f); // ghi dạng binary
									else break; // không nhận được byte nào (phía client đóng kết nối).
								}
								else break; // phía client đóng kết nối hoặc timeout.
							} while (true);
							closesocket(sdata); // đóng kết nối
							char sbuf[] = "226 Successful transfering.\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
							send(c, sbuf, strlen(sbuf), 0);
						}
						else {
							char sbuf[] = "425 Can't open data connection.\r\n";
							cout << "Send to " << c << ": " << sbuf << endl;
							send(c, sbuf, strlen(sbuf), 0);
						}
						trans_mode = 0;
						fclose(f);
					}
					else { // không mở được file để ghi
						char sbuf[] = "550 Cannot create file.\r\n";
						cout << "Send to " << c << ": " << sbuf << endl;
						send(c, sbuf, strlen(sbuf), 0);
					}
				}
				else {
					char sbuf[] = "550 Filename invalid.\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "RNFR") == 0) {
			if (!login) { // yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(args) == 0) { // không được bỏ trống tên thư mục
				char sbuf[] = "501 Syntax error\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (!(user->fperm & 1) || !(user->dperm & 2)) { // phải có quyền thực thi file và quyền ghi thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				int type = findFile(p_path);
				if (type == 1 || type == 2) {
					strcpy(fileReadyForRename, p_path);
					char sbuf[] = "350 File or directory exists, ready for destination name.\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					char sbuf[] = "550 File or directory not found.\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "RNTO") == 0) {
			if (!login) { // yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(args) == 0) { // không được bỏ trống tên thư mục
				char sbuf[] = "501 Syntax error\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(fileReadyForRename) == 0) { // phải gửi lệnh RNFR trước
				char sbuf[] = "503 Bad sequence command\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char tmp[PATH_LENGTH_MAX] = "";
				strcpy(tmp, fileReadyForRename);
				char * last = strrchr(tmp, '/');
				*(last + 1) = '\0'; 
				strcat(tmp, args);
				if (rename(fileReadyForRename, tmp) == 0) { // hàm rename có sẵn trong c
					char sbuf[] = "250 file renamed successfully\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
					fileReadyForRename[0] = '\0';
				}
				else {
					char sbuf[1024] = "";
					sprintf(sbuf, "550 %s\r\n", strerror(errno)); // lấy lỗi khi sử dụng hàm rename 
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "DELE") == 0) {
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(args) == 0) { // không được bỏ trống tên
				char sbuf[] = "501 Syntax error\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (!(user->fperm & 1) || !(user->dperm & 2)) { // phải có quyền thực thi file và quyền ghi thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				if (remove(p_path) == 0) { // hàm remove có sẵn
					char sbuf[] = "250 Deleted successfully\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					char sbuf[1024] = "";
					sprintf(sbuf, "550 %s\r\n", strerror(errno)); // lấy lỗi khi sử dụng hàm remove
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "MKD") == 0) { // tạo thư mục mới
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strlen(args) == 0) { // không được bỏ trống tên thư mục
				char sbuf[] = "501 Syntax error\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (!(user->dperm & 2)) { // phải có quyền ghi thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				if (CreateDirectory(p_path, NULL)) {
					char sbuf[] = "257 Created directory successfully\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					char sbuf[] = "550 Cannot create directory\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "RMD") == 0) { // xóa thư mục trống
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (user->dperm & 3 != 3) { // phải có quyền thực thi thư mục và quyền ghi thư mục
				char sbuf[] = "550 Permission denied\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				if (RemoveDirectory(p_path)) {
					char sbuf[] = "250 Removed directory successfully\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					char sbuf[] = "550 Cannot remove this directory\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "SYST") == 0) { // thông tin về hệ thống
			char sbuf[] = "215 UNIX\r\n";
			cout << "Send to " << c << ": " << sbuf << endl;
			send(c, sbuf, strlen(sbuf), 0);
		}
		else if (strcmp(cmd, "SIZE") == 0) { // size của file, thư mục thì báo lỗi	
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				char v_path[PATH_LENGTH_MAX] = "";
				char p_path[PATH_LENGTH_MAX] = "";
				processPathname(cwd, user->h_path, args, v_path, p_path);
				int size = sizeOfFile(p_path);
				if (size != 0) {
					char sbuf[1024] = "";
					sprintf(sbuf, "%d %d\r\n", 213, size);
					send(c, sbuf, strlen(sbuf), 0);
				}
				else {
					char sbuf[] = "550 File not found.\r\n";
					cout << "Send to " << c << ": " << sbuf << endl;
					send(c, sbuf, strlen(sbuf), 0);
				}
			}
		}
		else if (strcmp(cmd, "TYPE") == 0) { // kiểu dữ liệu biểu diễn
			if (!login) {
				// yêu cầu login 
				char sbuf[] = "530 Please login with USER and PASS!\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strcmp(args, "I") == 0) { // kiểu nhị phân
				data_type = BINARY_DATA_TYPE;
				char sbuf[] = "200 Type set to I.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strcmp(args, "A") == 0) { 
				// gửi dữ liệu dạng ascii chuẩn dành cho tệp văn bản
				// ở dạng ascii chuẩn thì kí tự CRLF là kí tự xuống dòng - giống hđh Windows, còn Unix thì chỉ là LF
				// phía bên nhận và gửi dữ liệu phải chuyển dữ liệu thành dạng ascii chuẩn đề gửi đi và chuyển thành dạng phù hợp trên hđh khi nhận đc dữ liệu
				data_type = ASCII_DATA_TYPE;
				char sbuf[] = "200 Type set to A.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else if (strcmp(args, "E") == 0 || strcmp(args, "L") == 0) {
				// không hỗ trợ 2 kiểu này
				char sbuf[] = "421 Server does not support this data type.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
			else {
				// lỗi tham số
				char sbuf[] = "501 Syntax error.\r\n";
				cout << "Send to " << c << ": " << sbuf << endl;
				send(c, sbuf, strlen(sbuf), 0);
			}
		}
		else if (strcmp(cmd, "QUIT") == 0) { // kết thúc phiên
			char sbuf[] = "221 Goodbye.\r\n";
			cout << "Send to " << c << ": " << sbuf << endl;
			send(c, sbuf, strlen(sbuf), 0);
			break;

		}
		else {	// lenh khong duoc xu ly -> loi cu phap
			char sbuf[] = "500 Systax error, command unrecognized.\r\n";
			cout << "Send to " << c << ": " << sbuf << endl;
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
		cnum++;
		int client_info[6] = {};
		client_info[0] = c;
		client_info[1] = caddr.sin_addr.S_un.S_un_b.s_b1;
		client_info[2] = caddr.sin_addr.S_un.S_un_b.s_b2;
		client_info[3] = caddr.sin_addr.S_un.S_un_b.s_b3;
		client_info[4] = caddr.sin_addr.S_un.S_un_b.s_b4;
		client_info[5] = caddr.sin_port;
		CreateThread(NULL, 0, clientThread, (LPVOID)client_info, 0, NULL);
	}
	clear();
	system("pause");
	return 0;
}