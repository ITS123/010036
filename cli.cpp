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
#define SEND_INDEX_MAX 1
#define END_SIGN 0

Mat make_end_image(int ROW, int COL);
void send_image(Mat end_image,int client_socket, int *p, double size);

int main( int argc, char *argv[])
{
    int client_socket;
    struct sockaddr_in server_addr;

    VideoCapture cap("test_video.mp4");
    double fps = cap.get(CAP_PROP_FPS);
    int delay = cvRound(1000/(int)fps);

    Mat img,sample;
    cap >> sample;
    int image_bus[COL_SIZE*3] = {0};
    int size_bus[3] = {htonl(ROW_SIZE), htonl(COL_SIZE), htonl(sample.type())};
    short send_index = 0;

    client_socket  = socket(AF_INET, SOCK_STREAM, 0);
    if( -1 == client_socket){ puts("소켓 생성 실패"); exit(1); }
    else puts("소켓 생성");

    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if( -1 == connect( client_socket, (struct sockaddr*)&server_addr, sizeof( server_addr))){ puts("접속 : Fail"); exit(1); }
    else puts("접속 : Success");    

    if(send(client_socket, size_bus, sizeof(size_bus),0) < 0) printf("send error/n");   

    while(1)
    {
        cap.read(img);
        if(img.empty())
        {
            Mat end_image(ROW_SIZE, COL_SIZE, 16);
            end_image = make_end_image(ROW_SIZE, COL_SIZE);
            for(short a=0; a<5;a++)
                send_image(end_image, client_socket, image_bus, sizeof(image_bus));    

            printf("send end sign\n");
            imshow("img_client",end_image);
            break;
        }

        resize(img, img, Size(COL_SIZE, ROW_SIZE),0,0,CV_INTER_NN);

        if(send_index == 0)
        {
            send_image(img, client_socket, image_bus, sizeof(image_bus));
            send_index++;            
        } 
        else
        {
            if (send_index == SEND_INDEX_MAX) send_index = 0;
            else send_index++;
            send_index = 0;
        }            
        imshow("img_client",img);
        if(waitKey(delay) == 27) break;
    }
    cap.release();
    destroyAllWindows();
    if(close(client_socket) == 0) printf("소켓 닫음\n");
    return 0;
}

Mat make_end_image(int ROW, int COL)
{
    Mat end_image(ROW_SIZE, COL_SIZE, 16);

     for (short i = 0; i < ROW_SIZE; i++)
    {
        uchar* p_img = end_image.ptr<uchar>(i);

        for (short j = 0; j < COL_SIZE; j++)
        {
            p_img[3*j+0] = 101;
            p_img[3*j+1] = 110;
            p_img[3*j+2] = 100;
        }
    }
    return end_image;
}

void send_image(Mat end_image,int client_socket, int *p, double size)
{
    for (short i = 0; i < ROW_SIZE; i++)
    {
        uchar* p_img = end_image.ptr<uchar>(i);

        for (short j = 0; j < COL_SIZE; j++)
        {
            p[3*j] = htonl(p_img[j*3+0]);
            p[3*j+1] = htonl(p_img[j*3+1]);
            p[3*j+2] = htonl(p_img[j*3+2]);             
        }
        send(client_socket, p, size,0);                               
    }                                
}