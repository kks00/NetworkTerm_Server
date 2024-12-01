#include "Global.h"

SOCKETINFO* SocketInfoList;

// 윈도우 메시지 처리 함수
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ProcessTCPSocketMessage(HWND, UINT, WPARAM, LPARAM);
void ProcessUDPSocketMessage(HWND, UINT, WPARAM, LPARAM);

int main(int argc, char *argv[])
{
	int retval;

	// 윈도우 클래스 등록
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = NULL;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = L"MyWndClass";
	if(!RegisterClass(&wndclass)) return 1;

	// 윈도우 생성
	HWND hWnd = CreateWindowA("MyWndClass", "서버", WS_OVERLAPPEDWINDOW,
		0, 0, 600, 200, NULL, NULL, NULL, NULL);
	if(hWnd == NULL) return 1;
	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");

	// WSAAsyncSelect()
	retval = WSAAsyncSelect(listen_sock, hWnd,
		WM_SOCKET, FD_ACCEPT|FD_CLOSE);
	if(retval == SOCKET_ERROR) err_quit("WSAAsyncSelect()");


	/*** UDP 서버 코드 시작 ***/
	// socket()
	SOCKET udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(udp_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// WSAAsyncSelect()
	retval = WSAAsyncSelect(udp_sock, hWnd,
		WM_UDP_SOCKET, FD_READ);
	if(retval == SOCKET_ERROR) err_quit("WSAAsyncSelect()");
	/*** UDP 서버 코드 끝 ***/

	// 메시지 루프
	MSG msg;
	while(GetMessage(&msg, 0, 0, 0) > 0){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// 윈속 종료
	WSACleanup();
	return msg.wParam;
}

// 윈도우 메시지 처리
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
	case WM_SOCKET: // TCP 소켓 관련 윈도우 메시지
		ProcessTCPSocketMessage(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_UDP_SOCKET: /*** UDP 소켓 관련 윈도우 메시지 ***/
		ProcessUDPSocketMessage(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// TCP 소켓 관련 윈도우 메시지 처리
void ProcessTCPSocketMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// 데이터 통신에 사용할 변수
	SOCKETINFO *ptr;
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen, retval;
	MessageInfo message_info;

	// 오류 발생 여부 확인
	if(WSAGETSELECTERROR(lParam)){
		err_display(WSAGETSELECTERROR(lParam));
		RemoveSocketInfo(wParam);
		return;
	}

	// 메시지 처리
	switch(WSAGETSELECTEVENT(lParam)){
	case FD_ACCEPT:
		addrlen = sizeof(clientaddr);
		client_sock = accept(wParam, (SOCKADDR *)&clientaddr, &addrlen);
		if(client_sock == INVALID_SOCKET){
			err_display("accept()");
			return;
		}
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		AddSocketInfo(client_sock, "");
		retval = WSAAsyncSelect(client_sock, hWnd,
			WM_SOCKET, FD_READ|FD_WRITE|FD_CLOSE);
		if(retval == SOCKET_ERROR){
			err_display("WSAAsyncSelect()");
			RemoveSocketInfo(client_sock);
		}
		break;

	case FD_READ:
		ptr = GetSocketInfo(wParam);
		if (!ptr) {
			printf("[%s] Socket 정보를 찾을 수 없습니다.", __func__);
			return;
		}
		message_info = ptr->last_message_info;

		// 메시지 정보를 받은 상태라면 페이로드 수신으로 점프
		if (ptr->is_info_received)
			goto receive_payload;

		// 고정길이(8바이트)의 메시지 정보(메세지 타입, 페이로드 길이)를 받음
		memset(&ptr->last_message_info, 0, sizeof(MessageInfo));
		retval = recv(ptr->sock, (char*)&ptr->last_message_info, sizeof(MessageInfo), 0);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				return;
			else {
				err_display("[ProcessTCPSocketMessage] recv message info");
				RemoveSocketInfo(wParam);
				return;
			}
		}
		ptr->is_info_received = true;
		ptr->recv_bytes = 0;

		// 메모리 동적할당
		ptr->recv_buf = (char*)VirtualAlloc(NULL, 0xFFFFFFF, MEM_COMMIT, PAGE_READWRITE);
		if (!ptr->recv_buf) {
			err_display("[ProcessTCPSocketMessage] alloc");
			return;
		}

		// 받은 메세지 정보 출력
		addrlen = sizeof(clientaddr);
		getpeername(wParam, (SOCKADDR*)&clientaddr, &addrlen);
		printf("[TCP/%s:%d] RecvBytes: %d Type: %d Length: %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
			retval, ptr->last_message_info.payload_type, ptr->last_message_info.payload_length);

		return;

	receive_payload:
		// 받은 페이로드 크기만큼 가변 길이 페이로드 받기
		retval = recv(ptr->sock, ptr->recv_buf + ptr->recv_bytes, ptr->last_message_info.payload_length - ptr->recv_bytes, 0);
		if (retval == SOCKET_ERROR) {
			err_display("[ProcessTCPSocketMessage] recv payload");
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				return;
			}
			else {
				RemoveSocketInfo(wParam);
				return;
			}
		}
		ptr->recv_bytes += retval;
		if (ptr->recv_bytes < ptr->last_message_info.payload_length)
			return;

		// 모두 수신완료 되었을 때 처리 시작
		ptr->is_info_received = false;

		// 받은 메세지 출력
#ifdef LOG_PACKET_RAW
		printf("Payload: %s\n", byteArrayToHexString(ptr->recv_buf, message_info.payload_length).c_str());
#endif

		if (ptr->recv_buf) {

			if (message_info.payload_type == SET_USER_NAME) { // 초기 이름설정 처리
				ptr->user_id = (char*)ptr->recv_buf; // 이름을 소켓 구조체에 저장해두기
				printf("사용자 이름: %s\n", ptr->user_id.c_str());

				// [아이디] 님이 입장했습니다 메시지 전송
				char chat_msg[BUFSIZE];
				sprintf_s(chat_msg, "[%s] 님이 입장했습니다.", ptr->user_id.c_str());
				tcp_send_to_all(CHATTING, chat_msg, strlen(chat_msg) + 1);

				// 접속중인 모든 클라이언트에게 현재 접속중인 유저 정보 전송
				send_clients_info();
			}

			else if (message_info.payload_type == CHATTING) { // 전체 채팅 처리
				char chat_msg[BUFSIZE];
				sprintf_s(chat_msg, "[%s] %s", ptr->user_id.c_str(), ptr->recv_buf); // [아이디] 채팅 형식으로 전송
				tcp_send_to_all(message_info.payload_type, chat_msg, strlen(chat_msg) + 1);
			}

			else if (message_info.payload_type == SEND_WHISP) { // 귓속말 전송 처리
				SEND_WHISP_DATA* recv_whisp_data = (SEND_WHISP_DATA *)ptr->recv_buf;

				char chat_msg[BUFSIZE];
				sprintf_s(chat_msg, "귓속말 [%s -> %s] %s", ptr->user_id.c_str(), recv_whisp_data->sender_id, recv_whisp_data->message);

				// 타겟과 센더에게만 채팅 메시지 전송
				tcp_send_to_target((char *)ptr->user_id.c_str(), CHATTING, chat_msg, strlen(chat_msg) + 1);
				tcp_send_to_target(recv_whisp_data->sender_id, CHATTING, chat_msg, strlen(chat_msg) + 1);
			}

			else {
				// 이외 경우 접속해있는 모든 클라이언트에게 받은 데이터 그대로 전송
				tcp_send_to_all(message_info.payload_type, (char*)ptr->recv_buf, message_info.payload_length);
			}

			// 처리 후 버퍼 할당해제
			VirtualFree(ptr->recv_buf, 0, MEM_RELEASE);
			ptr->recv_buf = NULL;
		}
		break;
	case FD_WRITE:
		break;
	case FD_CLOSE:
		// 클라이언트 종료시 소켓 정보 제거
		RemoveSocketInfo(wParam);
		break;
	}
}

/*** UDP 소켓 관련 윈도우 메시지 처리 ***/
void ProcessUDPSocketMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// 데이터 통신에 사용할 변수
	SOCKET sock = (SOCKET)wParam;
	SOCKADDR_IN clientaddr;
	int addrlen, retval;
	char buf[BUFSIZE+1];

	// 오류 발생 여부 확인
	if(WSAGETSELECTERROR(lParam)){
		err_display(WSAGETSELECTERROR(lParam));
		return;
	}

	// 메시지 처리
	switch (WSAGETSELECTEVENT(lParam)) {
	case FD_READ:
		// 데이터 받기
		MessageInfo message_info;
		// 고정길이(8바이트)의 메세지 정보(메세지 타입, 페이로드 길이)를 수신
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, (char*)&message_info, sizeof(message_info), 0, (sockaddr*)&clientaddr, &addrlen);
		if (retval == 0 || retval == SOCKET_ERROR) {
			return;
		}

		printf("[UDP/%s:%d] Type: %d Length: %d\n", inet_ntoa(clientaddr.sin_addr),
			ntohs(clientaddr.sin_port), message_info.payload_type, message_info.payload_length);


		// 페이로드 크기만큼 메모리 동적할당
		char* recv_buf = (char*)malloc(message_info.payload_length);
		if (!recv_buf) {
			return;
		}

		
	recv_payload:
		// 받은 페이로드 크기만큼 가변 길이 페이로드 받기
		retval = recvfrom(sock, (char*)recv_buf, message_info.payload_length, 0, (sockaddr*)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) // 페이로드를 받을 때까지 계속 시도
				goto recv_payload;
			else {
				free(recv_buf);
				return;
			}
		}

		// 받은 데이터 출력
#ifdef LOG_PACKET_RAW
		printf("Payload: %s\n", byteArrayToHexString(recv_buf, message_info.payload_length).c_str());
#endif

		// 접속해있는 모든 클라이언트에게 TCP로 현재 받은 데이터 전송
		tcp_send_to_all(message_info.payload_type, (char*)recv_buf, message_info.payload_length);

		// 처리 후 버퍼 할당해제
		free(recv_buf);
		break;
	}
}