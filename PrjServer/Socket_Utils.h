#pragma once

#include "Global.h"

#define BUFSIZE       512
#define SERVERPORT    9000


// 메시지 타입 정의
#define MESSAGE_INFO 1100
#define SET_USER_NAME 1101
#define UPLOAD_IMAGE 1102
#define USER_LIST_DATA 1103
#define SEND_WHISP 1104
#define RECV_WHISP 1105
#define SEND_CHAT 1106
#define RECV_MESSAGE 1107

#define DRAW_LINE             1001			// 메시지 타입: 선
#define DRAW_STRAIGHTLINE     1002			// 메시지 타입: 직선
#define DRAW_ELLIPSE          1003			// 메시지 타입: 타원
#define DRAW_RECTANGLE        1004			// 메시지 타입: 사각형
#define DRAW_TRIANGLE         1005 			// 메시지 타입: 삼각형
#define DRAW_RIGHTTRIANGLE    1006 			// 메시지 타입: 직각 삼각형
#define DRAW_STAR             1007 			// 메시지 타입: 별
#define DRAW_PARALLELOGRAM    1008 			// 메시지 타입: 평행사변형
#define DRAW_DIAMOND          1009			// 메시지 타입: 마름모
#define DRAW_ARROW            1010			// 메시지 타입: 화살표

#define DRAW_ERASER           1011			// 메시지 타입: 지우개


#define WM_SOCKET     (WM_USER+1)
#define WM_UDP_SOCKET (WM_USER+2) /*** UDP 소켓을 위한 메시지를 별도로 정의한다. ***/

// 고정 길이 전송시 사용할 데이터 구조체
struct MessageInfo {
	unsigned int payload_type; // 메시지 타입
	unsigned int payload_length; // 뒤따라올 페이로드의 길이
};

// TCP 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	SOCKET sock;

	// 고정길이 메시지 정보 수신 후 페이로드를 수신할 때 사용할 정보
	bool is_info_received;
	MessageInfo last_message_info;
	char* recv_buf;
	size_t recv_bytes;

	string user_id; // 사용자 이름

	SOCKETINFO* next;
};
extern SOCKETINFO* SocketInfoList;


#define MSGSIZE     (BUFSIZE-sizeof(int))  // 채팅 메시지 최대 길이
// 채팅 메시지 형식
struct CHAT_MSG
{
	COLORREF color; // 메시지 색상
	char buf[MSGSIZE]; // 메시지 데이터
};

#define USERNAMESIZE 32 // 사용자 이름 최대길이
// 귓속말 전송 데이터 구조체 정의
struct SEND_WHISP_DATA {
	char sender_id[USERNAMESIZE];
	char message[MSGSIZE];
};


void tcp_send_to_all(int message_type, char* payload_buf, int payload_size);
void tcp_send_to_target(char* user_id, int message_type, char* payload_buf, int payload_size);

void send_clients_info();

// 소켓 관리 함수
BOOL AddSocketInfo(SOCKET sock, string id);
SOCKETINFO* GetSocketInfo(SOCKET sock);
void RemoveSocketInfo(SOCKET sock);

// 오류 출력 함수
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);