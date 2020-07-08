#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

int main(int argc, char *argv[])
{
    int server_socket;
    int client_socket; 
    int client_addr_size;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    //소켓 생성
    server_socket  = socket( AF_INET, SOCK_STREAM, 0);
    if( -1 == server_socket){
        puts("소켓 생성 실패"); exit(1);
    }
    else{
        puts("소켓 생성");
    }

    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr= htonl(INADDR_ANY);
 
    //바인딩
    if( -1 == bind( server_socket, (struct sockaddr*)&server_addr, sizeof( server_addr))){
        puts("bind : Fail"); exit(1);
    }
    else puts("bind : Success");
    

    //대기 상태 설정
    if( -1 == listen(server_socket, 5)){
        puts("요청 대기 실패"); exit(1);   
    }   
    else puts("요청 대기");



    //클라이언트 연결
    client_addr_size = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_size);
    if ( -1 == client_socket){
        puts( "클라이언트 연결 실패"); exit(1);
    }   
    else puts( "클라이언트 연결 성공");


    int32_t size_bus[3] = {0};
    if(recv(client_socket, size_bus, sizeof(size_bus), 0) < 0)
        printf("receive error");

    int32_t ROW = ntohl(size_bus[0]);
    int32_t COL = ntohl(size_bus[1]);
    int TYPE = ntohl(size_bus[2]);
    int32_t image_bus[COL*3] = {0};

    Mat img(ROW, COL, 16);
    int col = 0;
    int count = 0;
    
    printf("%d %d", ROW, COL);

    //데이터 수신
    while (1)
    {      
        if(recv(client_socket, image_bus, sizeof(image_bus), 0) < 0)
            printf("receive error");
        else{
            for(short j = 0; j<COL; j++){
                Vec3b& p1 = img.at<Vec3b>(count,j);

                p1[0] = ntohl(image_bus[3*j]);
                p1[1] = ntohl(image_bus[3*j+1]);
                p1[2] = ntohl(image_bus[3*j+2]);
                col++;
            }
        }
        ++count;
        if(count == ROW)
        {
            imshow("img_server",img);
            if(waitKey(30)==27) break;
            count = 0;
        }
        
    }
    waitKey();


    close(client_socket);
    close(server_socket);

    return 0;
}