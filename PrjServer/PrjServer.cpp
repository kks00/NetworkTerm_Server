#include "Global.h"

SOCKETINFO* SocketInfoList;
HWND g_UserList = NULL;
HWND g_NoticeText = NULL;

// 윈도우 메시지 처리 함수
BOOL CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void init_socket(HWND hWnd);
void ProcessTCPSocketMessage(HWND, UINT, WPARAM, LPARAM);
void ProcessUDPSocketMessage(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// 디버깅 모드일 시 콘솔 생성
#ifdef DEBUG_MODE
	if (AllocConsole()) {
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);

		setbuf(stdout, NULL);
	}
#endif

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, WndProc);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// TCP, UDP 소켓 생성함수
void init_socket(HWND hWnd) {
	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	int retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// WSAAsyncSelect()
	retval = WSAAsyncSelect(listen_sock, hWnd,
		WM_SOCKET, FD_ACCEPT | FD_CLOSE);
	if (retval == SOCKET_ERROR) err_quit("WSAAsyncSelect()");


	/*** UDP 서버 코드 시작 ***/
	// socket()
	SOCKET udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(udp_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// WSAAsyncSelect()
	retval = WSAAsyncSelect(udp_sock, hWnd,
		WM_UDP_SOCKET, FD_READ);
	if (retval == SOCKET_ERROR) err_quit("WSAAsyncSelect()");
	/*** UDP 서버 코드 끝 ***/
}

SOCKETINFO* get_selected_user(string &user_id) {
	// 선택된 항목의 인덱스를 가져오기
	LRESULT selIndex = SendMessage((HWND)g_UserList, LB_GETCURSEL, 0, 0);
	// 선택된 항목이 없는 경우 리턴
	if (selIndex == LB_ERR)
		return NULL;

	char selected_user_id[USERNAMESIZE] = { 0 };
	// 선택된 항목의 텍스트를 가져오기
	SendMessageA((HWND)g_UserList, LB_GETTEXT, selIndex, (LPARAM)selected_user_id);

	user_id = selected_user_id;
	printf("[%s] selected user id=%s\n", __func__, selected_user_id);

	SOCKETINFO* target_socket_info = GetSocketInfoByID(selected_user_id);
	if (!target_socket_info) {
		printf("[%s] 소켓 정보를 찾을 수 없습니다.\n", __func__);
		return NULL;
	}
	return target_socket_info;
}

void kick_user() {
	string user_id;
	SOCKETINFO* target_socket_info = get_selected_user(user_id);
	if (!target_socket_info)
		return;

	RemoveSocketInfo(target_socket_info->sock);

	// [아이디] 님이 강제 퇴장되었습니다 메시지 전송
	CHAT_MSG chat_msg;
	chat_msg.color = RGB(255, 0, 0); // 빨간색
	sprintf_s(chat_msg.buf, "[%s] 님이 강제 퇴장되었습니다.", user_id.c_str());
	tcp_send_to_all(RECV_MESSAGE, (char*)&chat_msg, sizeof(chat_msg));
}

void mute_user(bool toggle) {
	string user_id;
	SOCKETINFO* target_socket_info = get_selected_user(user_id);
	if (!target_socket_info)
		return;

	target_socket_info->is_muted = toggle;

	// 메시지 전송
	CHAT_MSG chat_msg;
	chat_msg.color = RGB(255, 0, 0); // 빨간색
	string status = (toggle) ? "금지" : "허용";
	sprintf_s(chat_msg.buf, "[%s] 님의 채팅이 %s되었습니다.", user_id.c_str(), status.c_str());
	tcp_send_to_all(RECV_MESSAGE, (char*)&chat_msg, sizeof(chat_msg));
}

// Dialog 이벤트 처리 프로시저
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		g_UserList = GetDlgItem(hDlg, IDC_USERLIST);
		g_NoticeText = GetDlgItem(hDlg, IDC_NOTICETEXT);
		init_socket(hDlg);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;

		case IDC_KICKUSER:
			kick_user();
			return TRUE;

		case IDC_MUTEUSER:
			mute_user(true);
			return TRUE;

		case IDC_UNMUTEUSER:
			mute_user(false);
			return TRUE;

		case IDC_NOTICETEXT:
			return TRUE;
		}
	}
	return FALSE;
}

// 윈도우 메시지 처리
BOOL CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
	case WM_INITDIALOG:
	case WM_COMMAND: // Dialog 관련 이벤트 처리
		return DlgProc(hWnd, uMsg, wParam, lParam);

	case WM_SOCKET: // TCP 소켓 관련 윈도우 메시지
		ProcessTCPSocketMessage(hWnd, uMsg, wParam, lParam);
		return TRUE;

	case WM_UDP_SOCKET: /*** UDP 소켓 관련 윈도우 메시지 ***/
		ProcessUDPSocketMessage(hWnd, uMsg, wParam, lParam);
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
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
			printf("[%s] Socket 정보를 찾을 수 없습니다.\n", __func__);
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

		// 페이로드 크기만큼 메모리 동적할당
		// malloc 사용시 크기가 큰 이미지파일 수신할 때 할당이 제대로 이루어지지 않아 Windows API 사용
		ptr->recv_buf = (char*)VirtualAlloc(NULL, 0x0FFFFFFF, MEM_COMMIT, PAGE_READWRITE);
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
			return; // 데이터 수신이 덜되었다면 대기

		// 모두 수신완료 되었을 때 처리 시작
		ptr->is_info_received = false;

		// 받은 메시지 출력
#ifdef LOG_PACKET_RAW
		printf("Payload: %s\n", byteArrayToHexString(ptr->recv_buf, message_info.payload_length).c_str());
#endif

		if (ptr->recv_buf) {

			if (message_info.payload_type == SET_USER_NAME) { // 초기 이름설정 처리
				ptr->user_id = (char*)ptr->recv_buf; // 이름을 소켓 구조체에 저장해두기
				printf("사용자 이름: %s\n", ptr->user_id.c_str());

				// [아이디] 님이 입장했습니다 메시지 전송
				CHAT_MSG chat_msg;
				chat_msg.color = RGB(0, 255, 0); // 초록색
				sprintf_s(chat_msg.buf, "[%s] 님이 입장했습니다.", ptr->user_id.c_str());
				tcp_send_to_all(RECV_MESSAGE, (char *)&chat_msg, sizeof(chat_msg));

				// 접속중인 모든 클라이언트에게 현재 접속중인 유저 정보 전송
				send_clients_info();
			}

			else if (message_info.payload_type == SEND_CHAT) { // 전체 채팅 처리
				CHAT_MSG* recv_chat_msg = (CHAT_MSG*)ptr->recv_buf;

				if (!ptr->is_muted) { // 채팅금지 상태가 아닐 때
					CHAT_MSG chat_msg;
					chat_msg.color = RGB(0, 0, 0); // 검은색
					sprintf_s(chat_msg.buf, "[%s] %s", ptr->user_id.c_str(), recv_chat_msg->buf); // [아이디] 채팅 형식으로 전송

					tcp_send_to_all(RECV_MESSAGE, (char*)&chat_msg, sizeof(chat_msg));
				}
				else { // 채팅금지 상태일 때
					CHAT_MSG chat_msg;
					chat_msg.color = RGB(255, 0, 0); // 빨간색
					strcpy(chat_msg.buf, "채팅금지 상태이므로 전체채팅을 보낼 수 없습니다.");
					tcp_send_to_target((char *)ptr->user_id.c_str(), RECV_MESSAGE, (char*)&chat_msg, sizeof(chat_msg));
				}
			}

			else if (message_info.payload_type == SEND_WHISP) { // 귓속말 전송 처리
				SEND_WHISP_DATA* recv_whisp_data = (SEND_WHISP_DATA *)ptr->recv_buf;

				CHAT_MSG chat_msg;
				chat_msg.color = RGB(0, 0, 255); // 파란색
				sprintf_s(chat_msg.buf, "귓속말 [%s -> %s] %s", ptr->user_id.c_str(), recv_whisp_data->sender_id, recv_whisp_data->message);

				// 타겟과 센더에게만 채팅 메시지 전송
				tcp_send_to_target((char *)ptr->user_id.c_str(), RECV_MESSAGE, (char*)&chat_msg, sizeof(chat_msg));
				tcp_send_to_target(recv_whisp_data->sender_id, RECV_MESSAGE, (char*)&chat_msg, sizeof(chat_msg));
			}

			else { // 이외 경우 접속해있는 모든 클라이언트에게 받은 데이터 그대로 전송
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
		// 고정길이(8바이트)의 메세지 정보(메세지 타입, 페이로드 길이)를 수신
		MessageInfo message_info;
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