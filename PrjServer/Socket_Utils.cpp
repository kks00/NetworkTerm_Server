#include "Socket_Utils.h"

// 접속한 모든 클라이언트에게 받은 데이터 보내기
void tcp_send_to_all(int message_type, char* payload_buf, int payload_size) {
	SOCKETINFO* ptr = SocketInfoList;
	while (ptr) {
		MessageInfo message_info;
		message_info.payload_length = payload_size;
		message_info.payload_type = message_type;

		// 먼저 고정길이(8바이트)의 메세지 정보(타입, 페이로드 길이)를 전송
		int retval = send(ptr->sock, (char*)&message_info, sizeof(message_info), 0);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				err_display("[tcp_send_to_all] send messageinfo");
			continue;
		}

		// 페이로드 전송
		retval = send(ptr->sock, payload_buf, message_info.payload_length, 0);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				err_display("[tcp_send_to_all] send payload");
			continue;
		}

		ptr = ptr->next;
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
	ptr->user_id = id;
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
			if (prev)
				prev->next = curr->next;
			else
				SocketInfoList = curr->next;
			closesocket(curr->sock);
			delete curr;
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