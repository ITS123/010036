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

#define ROW_SIZE 240
#define COL_SIZE 320

int main( int argc, char *argv[])
{
    int client_socket;                                                                  //소켓                          
    struct sockaddr_in server_addr;                                                     //서버 주소

    client_socket  = socket(AF_INET, SOCK_STREAM, 0);                                   //TCP 소켓 생성
    if( -1 == client_socket){ puts("소켓 생성 실패"); exit(1); }
    else puts("소켓 생성");

    memset( &server_addr, 0, sizeof( server_addr));                                     //통신할 서버 IP 할당
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if( -1 == connect( client_socket, \                                                 //접속 시도
                    (struct sockaddr*)&server_addr, \
                    sizeof( server_addr)))\
                    { puts("접속 : Fail"); exit(1); }

    else puts("접속 : Success");    

    uint32_t size_bus[2] = {htonl(ROW_SIZE), htonl(COL_SIZE)};                          //이미지 행렬 크기
    if(send(client_socket, size_bus, sizeof(size_bus),0) < 0) printf("send error/n");   //이미지 크기 전송

    VideoCapture cap("test_video.mp4");                                                 //영상 추출
    double fps = cap.get(CAP_PROP_FPS);                                                 //영상 프레임 추출
    int delay = cvRound(1000/(int)fps);                                                 //딜레이 설정

    while(1)
    {
        Mat img;                                                                        //전송할 이미지

        cap.read(img);                                                                  //프레임
        resize(img, img, Size(COL_SIZE, ROW_SIZE),0,0,CV_INTER_NN);                     //크기 조절

        if(! img.data )                                                                 //데이터 없을 시 에러
        {
            cout <<  "Could not open or find the image" << std::endl ;
            return -1;
        }

        int  imgSize = img.total()*img.elemSize();                                      //이미지의 총 데이터 크기
        send(client_socket, img.data, imgSize, 0);                                      //이미지 전송
        imshow("img_client",img);                                                       //전송한 이미지 출력
        if(waitKey((int)delay/5) == 27) break;                                          //딜레이
    }

    cap.release();
    destroyAllWindows();
    if(close(client_socket) == 0) printf("소켓 닫음\n");                                 //소켓 폐기
    return 0;
}
