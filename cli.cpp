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
#define SEND 6

int main( int argc, char *argv[])
{
    int send_timing = SEND;
    int client_socket;                          
    struct sockaddr_in server_addr;

    client_socket  = socket(AF_INET, SOCK_STREAM, 0);
    if( -1 == client_socket){ puts("소켓 생성 실패"); exit(1); }
    else puts("소켓 생성");

    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if( -1 == connect( client_socket, \
                    (struct sockaddr*)&server_addr, \
                    sizeof( server_addr)))\
                    { puts("접속 : Fail"); exit(1); }

    else puts("접속 : Success");    

    VideoCapture cap("test_video.mp4");
    double fps = cap.get(CAP_PROP_FPS);
    int delay = cvRound(1000/(int)fps);
    Mat sample;
    cap.read(sample);

    uint32_t size_bus[4] = {htonl(ROW_SIZE), htonl(COL_SIZE), htonl(sample.type()), htonl(sample.channels())};
    if(send(client_socket, size_bus, sizeof(size_bus),0) < 0) printf("send error/n");

    while(1)
    {
        Mat img;

        cap.read(img);
        resize(img, img, Size(COL_SIZE, ROW_SIZE),0,0,CV_INTER_NN);

        if(! img.data )
        {
            cout <<  "Could not open or find the image" << std::endl ;
            return -1;
        }
        
        if(send_timing == SEND)
        {
            for(short i = 0; i<ROW_SIZE; i++){
                Mat img_col;
                img_col = img(Range(i, i+1), Range(0,img.cols));
                int imgSize = img_col.total()*img.elemSize();     

                send(client_socket, img_col.data, imgSize, 0);
                send_timing = 0;
            }
        imshow("img_client",img);
        }
        
        send_timing++;
        if(waitKey((int)delay/5) == 27) break;
    }

    cap.release();
    destroyAllWindows();
    if(close(client_socket) == 0) printf("소켓 닫음\n");
    return 0;
}
