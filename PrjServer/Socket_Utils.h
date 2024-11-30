#pragma once

#include "Global.h"

#define BUFSIZE       512
#define SERVERPORT    9000


// 메세지 타입 정의
#define MESSAGE_INFO 1100
#define SET_USER_NAME 1101

#define CHATTING			  1000          // 메시지 타입: 채팅

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

struct MessageInfo {
	unsigned int payload_type;
	unsigned int payload_length;
};

// TCP 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	SOCKET sock;
	bool is_info_received;
	MessageInfo last_message_info;

	string user_id;

	SOCKETINFO* next;
};
extern SOCKETINFO* SocketInfoList;

// 선 그리기 메시지 형식
struct DRAWLINE_MSG
{
	int  type;
	int  color;
	int	 width;
	int	 line;
	int  x0, y0;
	int  x1, y1;
};


void tcp_send_to_all(int message_type, char* payload_buf, int payload_size);

// 소켓 관리 함수
BOOL AddSocketInfo(SOCKET sock, string id);
SOCKETINFO* GetSocketInfo(SOCKET sock);
void RemoveSocketInfo(SOCKET sock);

// 오류 출력 함수
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);