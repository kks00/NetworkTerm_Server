#include "Socket_Utils.h"

int send_tcp_payload(SOCKET sock, int message_type, char* payload_buf, int payload_size) {
	MessageInfo message_info;
	message_info.payload_length = payload_size;
	message_info.payload_type = message_type;

	// 먼저 고정길이(8바이트)의 메세지 정보(타입, 페이로드 길이)를 전송
	int retval = send(sock, (char*)&message_info, sizeof(message_info), 0);
	if (retval == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			err_display("[tcp_send_to_all] send messageinfo");
		return retval;
	}

	// 페이로드 전송
	retval = send(sock, payload_buf, message_info.payload_length, 0);
	if (retval == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			err_display("[tcp_send_to_all] send payload");
		return retval;
	}

	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(sock, (SOCKADDR*)&clientaddr, &addrlen);
	printf("[%s] sent %d bytes to %s:%d\n", __func__, retval, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	return retval;
}

// 접속한 모든 클라이언트에게 받은 데이터 보내기
void tcp_send_to_all(int message_type, char* payload_buf, int payload_size) {
	SOCKETINFO* ptr = SocketInfoList;
	while (ptr) { // 접속중인 모든 사용자 루프
		send_tcp_payload(ptr->sock, message_type, payload_buf, payload_size);
		ptr = ptr->next;
	}
}

// 접속한 클라이언트 중 ID가 일치하는 사용자에게만 전송
void tcp_send_to_target(char *user_id, int message_type, char* payload_buf, int payload_size) {
	SOCKETINFO* ptr = SocketInfoList;
	while (ptr) { // 접속중인 모든 사용자 루프
		if (!strcmp(user_id, ptr->user_id.c_str())) {
			send_tcp_payload(ptr->sock, message_type, payload_buf, payload_size);
		}
		ptr = ptr->next;
	}
}


// 접속중인 클라이언트 정보 전송
void send_clients_info() {
	string result = "";

	SOCKETINFO* ptr = SocketInfoList; // 접속중인 모든 사용자 루프
	while (ptr) {
		string curr_user_id = ptr->user_id;

		result += curr_user_id + "|"; // |을 구분자로 사용하여 접속중인 사용자 이름 붙이기

		ptr = ptr->next;
	}
	result.substr(0, result.length() - 1); // 마지막 한글자 떼기

	// 모두에게 전송
	tcp_send_to_all(USER_LIST_DATA, (char *)result.c_str(), result.length() + 1);

	// 서버의 유저 리스트 갱신
	SendMessage((HWND)g_UserList, LB_RESETCONTENT, 0, 0); // 리스트의 모든 항목 삭제

	char* item_text = strtok((char *)result.c_str(), "|");
	while (item_text != NULL) { // |를 기준으로 문자열 분리
		SendMessageA((HWND)g_UserList, LB_ADDSTRING, 0, (LPARAM)item_text); // 리스트에 데이터 추가
		item_text = strtok(NULL, "|");
	}
}


// 소켓 정보 추가
BOOL AddSocketInfo(SOCKET sock, string id)
{
	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		printf("[오류] 메모리가 부족합니다!\n");
		return FALSE;
	}

	ptr->sock = sock;

	ptr->is_info_received = false;
	memset(&ptr->last_message_info, 0, sizeof(MessageInfo));
	ptr->recv_buf = NULL;
	ptr->recv_bytes = 0;

	ptr->user_id = id;
	ptr->is_muted = false;

	ptr->next = SocketInfoList;
	SocketInfoList = ptr;

	printf("[%s] 소켓 정보가 추가되었습니다. SOCK=%x\n", __func__, ptr->sock);

	return TRUE;
}

// 소켓 정보 얻기
SOCKETINFO* GetSocketInfo(SOCKET sock)
{
	SOCKETINFO* ptr = SocketInfoList;

	while (ptr) {
		if (ptr->sock == sock)
			return ptr;
		ptr = ptr->next;
	}

	return NULL;
}

SOCKETINFO* GetSocketInfoByID(string user_id)
{
	SOCKETINFO* ptr = SocketInfoList;

	while (ptr) {
		if (!strcmp(user_id.c_str(), ptr->user_id.c_str()))
			return ptr;
		ptr = ptr->next;
	}

	return NULL;
}

// 소켓 정보 제거
void RemoveSocketInfo(SOCKET sock)
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(sock, (SOCKADDR*)&clientaddr, &addrlen);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

	SOCKETINFO* curr = SocketInfoList;
	SOCKETINFO* prev = NULL;

	while (curr) {
		if (curr->sock == sock) {
			string exit_user_id = curr->user_id;
			
			// 노드 삭제
			if (prev)
				prev->next = curr->next;
			else
				SocketInfoList = curr->next;
			closesocket(curr->sock);
			delete curr;

			// [아이디] 님이 퇴장했습니다 메시지 전송
			if (exit_user_id.length() > 0) {
				CHAT_MSG chat_msg;
				chat_msg.color = RGB(255, 0, 0); // 빨간색
				sprintf_s(chat_msg.buf, "[%s] 님이 퇴장했습니다.", exit_user_id.c_str());
				tcp_send_to_all(RECV_MESSAGE, (char*)&chat_msg, sizeof(chat_msg));
			}

			// 접속중인 모든 클라이언트에게 현재 접속중인 유저 정보 전송
			send_clients_info();

			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

// 소켓 함수 오류 출력 후 종료
void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	MessageBoxA(NULL, (LPCSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 소켓 함수 오류 출력
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	printf("[오류] %s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}