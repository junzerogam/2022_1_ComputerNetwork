#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8002
#define SIZE 1024
#define IP_ADDR "127.0.0.1"			
#define MAX_ROOM 3
#define MAX_WAIITNG_USER 5
#define MAX_ROOM_USER 5
#define MENU "<MENU>\n1. 채팅방 목록 보기\n2, 채팅방 참여하기(사용법 : 2 <채팅방 번호>)\n3. 프로그램 종료\n(0을 입력하면 메뉴가 다시 표시됩니다)\n"


struct ChatInfo {
	int chatting[MAX_WAIITNG_USER];
	int user_count;
};

struct ChatInfo chatRoom[MAX_ROOM];
pthread_t thread_id[MAX_ROOM];

int waittingRoomUser[MAX_WAIITNG_USER];
int waittingUserCount = 0;
int serverSk;

void sighandler(int signal);
void send_to_All(char *chat, int *arr, int size);
void *thread_func(void *arg);
void exit_thread(int siginal);
int maxArr(int *input, int size);
void exitArr(int *input, int *size, int value);

int main(){

	int waittingRoomSk[MAX_WAIITNG_USER]; // 대기실에 입장할 소켓들 저장
	int userCount = 0; // 접속 시도하려는 소켓들
	int strlength;
	char chat[SIZE];

	int serverSk_len, waittingSk_len;
	int chatRoomNum;

	fd_set reads;
	struct timeval time;
	int roomNum_thread[3] = {0 ,1 ,2}; // 채팅방 id

	// For Select
	int select_num;
	int select_max = -1;

	struct sockaddr_in serverSk_addr, waitting_addr;
	
	// INET Socket (Waitting Room Server - Monitor)
	if ((serverSk = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}

	memset(&serverSk_addr, 0, sizeof(serverSk_addr));
	serverSk_addr.sin_family = AF_INET;
	serverSk_addr.sin_port = htons(PORT);
	serverSk_addr.sin_addr.s_addr = inet_addr(IP_ADDR);

	if (bind(serverSk, (struct sockaddr *)&serverSk_addr, sizeof(serverSk_addr)) == -1){
		perror("bind");
		exit(1);
	}

	if (listen(serverSk, 5) < 0){
		perror("listen");
		exit(1);
	}

	// Set Non Blocking
	int inetFlags = fcntl(serverSk, F_GETFL, 0);
	fcntl(serverSk, F_SETFL, inetFlags | O_NONBLOCK);	

	// Create ChatRoom Thread
	for(int i= 0; i < MAX_ROOM; i++){
		printf("[INFO] Create ChatRoom #%d\n", i);
		pthread_create(&thread_id[i], NULL, thread_func, (void *)&roomNum_thread[i]);
	}

	FD_ZERO(&reads);
	// Signal Handler
	signal(SIGINT, sighandler);
	while(1){
		// 채팅시도하려는 유저가 대기실에서 대기
		waittingRoomSk[userCount] = accept(serverSk, (struct sockaddr *)&waitting_addr,&waittingSk_len);

		if(waittingRoomSk[userCount] > 0){
			// 최초 접속시 메뉴 전송
			if (send(waittingRoomSk[userCount], MENU, SIZE, 0) == -1){
				perror("send");
				exit(1);
			}

			waittingRoomUser[waittingUserCount] = waittingRoomSk[userCount];
			printf("[MAIN] Joined NEW Users : %d\n", waittingRoomUser[waittingUserCount]);
			
			userCount++; // 접속한 유저 증가
			waittingUserCount++; // 대기실에 접속 유저 증가

		}

		for (int i = 0; i < waittingUserCount; i++){
			FD_SET(waittingRoomUser[i], &reads);
		}

		time.tv_sec = 1;
		time.tv_usec = 1000;

		// For Select
		select_max = maxArr(waittingRoomUser, waittingUserCount);
		if ((select_num = select(select_max + 1, &reads, 0, 0, &time)) == -1){
			perror("select");
			continue;
		}
		if (select_num == 0){
			continue;
		}

		for (int who = 4; who < select_max + 1; who++){

			if (FD_ISSET(who, &reads)){
				
				strlength = recv(who, chat, SIZE, 0);
				chat[strlength] = 0;
				printf("[MAIN] Client [%d] MSG : %s\n", who, chat);
				switch(chat[0]) {
					// 0번 입력시 메뉴 제전달
					case '0' :
						if (send(who, MENU, SIZE, 0) == -1){
							perror("send");
							exit(1);
						}
						break;
					// 1번 입력시 현재 채팅방 상황 전송
					case '1' :
						strcpy(chat,"<CHAT ROOM INFO>\n");
						for (int current = 0; current < MAX_ROOM; current++){
							sprintf(chat, "%s[ID : %d] Chatroom-%d (%d/%d)\n", chat, current, current, chatRoom[current].user_count, MAX_ROOM_USER);
						}

						if (send(who, chat, SIZE, 0) == -1){
							perror("send");
							exit(1);
						}
						break;
					// 2번 입력시 해당 채팅방 입장
					case '2' :
						chatRoomNum = chat[2] - '0'; // 입장하려는 채팅방
						printf("[MAIN] Enter ChatRoom [Ch.%d]\n", chatRoomNum);
						// 채팅방 id에 따라 입장
						chatRoom[chatRoomNum].chatting[chatRoom[chatRoomNum].user_count] = who;
						// 채팅방 현재 몇명 있는지
						chatRoom[chatRoomNum].user_count++;

						printf("[Ch.%d] Current Chatting Users Count : %d Now\n", chatRoomNum, chatRoom[chatRoomNum].user_count);
						
						FD_CLR(who, &reads);
						printf("[MAIN] Chatter %d Join [Ch.%d]\n", who, chatRoomNum);
						// 대기실에서 채팅방으로 이동 했으니까 대기실에서 삭제
						exitArr(waittingRoomUser, &waittingUserCount, who);
						break;
					case '3' :
						// 채팅 대기실에서 나감
						printf("[MAIN] Client [%d] EXIT\n", who);
						FD_CLR(who, &reads);
						exitArr(waittingRoomUser, &waittingUserCount, who);
						close(who);
						break;
				}
			}
		}
	}
	close(serverSk);
}

// Just Chatting!!
void *thread_func(void *arg){

	int roomNum;
	int select_num, select_max, strlength;
	char chat[SIZE] ,sendAll[SIZE];

	fd_set reads;
	struct timeval time;
	
	roomNum = *((int *)arg);

	while(1) {
		FD_ZERO(&reads);
		time.tv_sec = 1;
		time.tv_usec = 1000;
		signal(SIGUSR1, exit_thread);

		// 채팅방 생성
		for(int i = 0; i < chatRoom[roomNum].user_count; i++){
			FD_SET(chatRoom[roomNum].chatting[i], &reads);
		}

		select_max = maxArr(chatRoom[roomNum].chatting, chatRoom[roomNum].user_count);
		if ((select_num = select(select_max + 1, &reads, 0, 0, &time)) == -1){
			perror("select");
			continue;
		}

		for( int who = 0; who < chatRoom[roomNum].user_count; who++){

			if(FD_ISSET(chatRoom[roomNum].chatting[who], &reads)){
				strlength = recv(chatRoom[roomNum].chatting[who], chat, SIZE,0);
				chat[strlength] = 0;
				printf("[Ch.%d] Client[%d] : %s\n", roomNum, chatRoom[roomNum].chatting[who], chat);
				// 채팅방 접속해 있는 유저 모두에게 전송
				sprintf(sendAll, "[%d] : %s", chatRoom[roomNum].chatting[who], chat);
				send_to_All(sendAll, chatRoom[roomNum].chatting, chatRoom[roomNum].user_count);

				// quit 메세지 입력시 채팅방 탈출 = 대기실로 이동 
				if (strcmp(chat, "quit\n") == 0){
					printf("[Ch.%d] Client[%d] EXIT\n", roomNum, chatRoom[roomNum].chatting[who]);
					waittingRoomUser[waittingUserCount] = chatRoom[roomNum].chatting[who];
					waittingUserCount++;
					exitArr(chatRoom[roomNum].chatting, &chatRoom[roomNum].user_count, chatRoom[roomNum].chatting[who]);
				}
			}
		}
	}
}

// 모두에게 메세지 전송
void send_to_All(char *chat, int *arr, int size){
	
	for (int i = 0; i < size; i++){
		if (send(arr[i], chat, SIZE, 0) == -1){
			perror("send");
			exit(1);
		}
	}
}

// SELECT 활용
int maxArr( int *input, int size){
	int temp = -1;

	for (int i = 0; i < size; i++){
		if (temp < input[i])
			temp = input[i];
	}

	return temp;
}

// 대기실 -> 채팅방 OR 채팅방 -> 대기실 OR 대기실에서도 탈출
void exitArr(int *input, int *size, int value){
	int index;

	for (int i = 0; i < *size; i++){
		if (input[i] == value){
			index  = i;
			break;
		}
	}

	for (int k = index; k < *size -1; k++){
		input[k] = input[k+1];
	}
	(*size)--;
}

// signal handler
void sighandler(int signal){

	printf(" (시그널 핸들러) 마무리 작업 시작\n");

	printf("> 마무리 과정 1/3 (thread exit)\n");

    printf("> 마무리 과정 2/3 (thread join)\n");
    for(int i = 0; i < MAX_ROOM; i++){
        pthread_kill(thread_id[i], SIGUSR1);
        pthread_join(thread_id[i], 0);
    }
    printf("> 마무리 과정 3/3 (close socket)\n");
    close(serverSk);

    
    exit(0);

}

void exit_thread(int sig){
	pthread_exit(0);
}








