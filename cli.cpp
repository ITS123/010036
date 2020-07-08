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

int main( int argc, char *argv[])
{
    int client_socket;
    struct sockaddr_in server_addr;

    client_socket  = socket(AF_INET, SOCK_STREAM, 0);
    if( -1 == client_socket){
        puts("소켓 생성 실패"); exit(1);
    }
    else puts("소켓 생성");

    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if( -1 == connect( client_socket,
                     (struct sockaddr*)&server_addr,
                      sizeof( server_addr))){
        puts("접속 : Fail"); exit(1);
    }
    else puts("접속 : Success");

    VideoCapture cap("test_video.mp4");
    Mat img,sample;

    cap >> sample;
    int ROW = sample.rows;
    int COL = sample.cols;
    printf("%d %d", ROW, COL);
    double fps = cap.get(CAP_PROP_FPS);
    int32_t image_bus[COL*3] = {0};
    int32_t size_bus[3] = {htonl(ROW), htonl(COL), htonl(sample.type())};

    if(send(client_socket, size_bus, sizeof(size_bus),0) < 0) 
        printf("send error/n");   
    
    while(1){
        cap >> img;

        for (short i = 0; i < ROW; i++)
        {
            for (short j = 0; j < COL; j++){
                Vec3b& p1 = img.at<Vec3b>(i, j);

                image_bus[3*j] = htonl(p1[0]);
                image_bus[3*j+1] = htonl(p1[1]);
                image_bus[3*j+2] = htonl(p1[2]);
            }   
            if(send(client_socket, image_bus, sizeof(image_bus),0) < 0) 
                printf("send error/n");    
        }
        imshow("img_client",img);
        if(waitKey(30)==27) break;
    }


    close(client_socket);
    return 0;
}