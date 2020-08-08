//#ifdef OPENCV

#include "stdio.h"
#include "stdlib.h"
#include "opencv2/opencv.hpp"
#include "image.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace cv;

extern "C" {

IplImage *image_to_ipl(image im)
{
    int x,y,c;
    IplImage *disp = cvCreateImage(cvSize(im.w,im.h), IPL_DEPTH_8U, im.c);
    int step = disp->widthStep;
    for(y = 0; y < im.h; ++y){
        for(x = 0; x < im.w; ++x){
            for(c= 0; c < im.c; ++c){
                float val = im.data[c*im.h*im.w + y*im.w + x];
                disp->imageData[y*step + x*im.c + c] = (unsigned char)(val*255);
            }
        }
    }
    return disp;
}

image ipl_to_image(IplImage* src)
{
    int h = src->height;
    int w = src->width;
    int c = src->nChannels;
    image im = make_image(w, h, c);
    unsigned char *data = (unsigned char *)src->imageData;
    int step = src->widthStep;
    int i, j, k;

    for(i = 0; i < h; ++i){
        for(k= 0; k < c; ++k){
            for(j = 0; j < w; ++j){
                im.data[k*w*h + i*w + j] = data[i*step + j*c + k]/255.;
            }
        }
    }
    return im;
}

Mat image_to_mat(image im)
{
    image copy = copy_image(im);
    constrain_image(copy);
    if(im.c == 3) rgbgr_image(copy);

    IplImage *ipl = image_to_ipl(copy);
    Mat m = cvarrToMat(ipl, true);
    cvReleaseImage(&ipl);
    free_image(copy);
    return m;
}

image mat_to_image(Mat m)
{
    IplImage ipl = m;
    image im = ipl_to_image(&ipl);
    rgbgr_image(im);
    return im;
}

void *open_video_stream(const char *f, int c, int w, int h, int fps)
{
    VideoCapture *cap;
    if(f) cap = new VideoCapture(f);
    else cap = new VideoCapture(c);
    if(!cap->isOpened()) return 0;
    if(w) cap->set(CV_CAP_PROP_FRAME_WIDTH, w);
    if(h) cap->set(CV_CAP_PROP_FRAME_HEIGHT, w);
    if(fps) cap->set(CV_CAP_PROP_FPS, w);
    return (void *) cap;
}

image get_image_from_stream(void *p)
{
    VideoCapture *cap = (VideoCapture *)p;
    Mat m;
    *cap >> m;
    if(m.empty()) return make_empty_image(0,0,0);
    return mat_to_image(m);
}

image load_image_cv_soc(int ROW, int COL)
{
    Mat img(ROW, COL, 16);

    for(short i=0; i<ROW; i++)
    {
        for(short j=0; j<COL; j++)
        {
                Vec3b& p1 = img.at<Vec3b>(i,j);

                p1[0] = 0;
                p1[1] = 0;
                p1[2] = 0;
        }
    }
    image im = mat_to_image(img);
    return im;
}

image load_image_cv(char *filename, int channels)
{
    int flag = -1;
    if (channels == 0) flag = -1;
    else if (channels == 1) flag = 0;
    else if (channels == 3) flag = 1;
    else {
        fprintf(stderr, "OpenCV can't force load with %d channels\n", channels);
    }
    Mat m;
    m = imread(filename, flag);
    if(!m.data){
        fprintf(stderr, "Cannot load image \"%s\"\n", filename);
        char buff[256];
        sprintf(buff, "echo %s >> bad.list", filename);
        system(buff);
        return make_image(10,10,3);
        //exit(0);
    }
    image im = mat_to_image(m);
    return im;
}

int show_image_cv(image im, const char* name, int ms)
{
    Mat m = image_to_mat(im);
    imshow(name, m);
    int c = waitKey(ms);
    if (c != -1) c = c%256;
    return c;
}

void make_window(char *name, int w, int h, int fullscreen)
{
    namedWindow(name, WINDOW_NORMAL); 
    if (fullscreen) {
        setWindowProperty(name, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    } else {
        resizeWindow(name, w, h);
        if(strcmp(name, "Demo") == 0) moveWindow(name, 0, 0);
    }
}

void socket_detector(char *datacfg, char *cfgfile, char *weightfile, char *filename, float thresh, float hier_thresh, char *outfile, int fullscreen)
{
    list *options = read_data_cfg(datacfg);
    char *name_list = option_find_str(options, "names", "data/names.list");
    char **names = get_labels(name_list);

    image **alphabet = load_alphabet();
    network *net = load_network(cfgfile, weightfile, 0);
    set_batch_network(net, 1);
    srand(2222222);
    double time;
    char buff[256];
    char *input = buff;
    float nms=.45;

    int server_socket;
    int client_socket; 
    int client_addr_size;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    //소켓 생성
    server_socket  = socket( AF_INET, SOCK_STREAM, 0);
    if( -1 == server_socket){ puts("소켓 생성 실패"); exit(1); }
    else puts("소켓 생성");

    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(11111);
    server_addr.sin_addr.s_addr= htonl(INADDR_ANY);
 
    //바인딩
    if( -1 == bind( server_socket, (struct sockaddr*)&server_addr, sizeof( server_addr)))
    { puts("bind : Fail"); exit(1); }
    else puts("bind : Success");

    //대기 상태 설정
    if( -1 == listen(server_socket, 5)) { puts("요청 대기 실패"); exit(1); }   
    else puts("요청 대기");

    //클라이언트 연결
    client_addr_size = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_size);
    if ( -1 == client_socket){ puts( "클라이언트 연결 실패"); exit(1); }   
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
            image im = mat_to_image(img);
                
            image sized = letterbox_image(im, net->w, net->h);
            layer l = net->layers[net->n-1];


            float *X = sized.data;
            time=what_time_is_it_now();
            network_predict(net, X);
            printf("%s: Predicted in %f seconds.\n", input, what_time_is_it_now()-time);
            int nboxes = 0;
            detection *dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);

            if (nms) do_nms_sort(dets, nboxes, l.classes, nms);
            draw_detections(im, dets, nboxes, thresh, names, alphabet, l.classes);
            free_detections(dets, nboxes);

            Mat result = image_to_mat(im);
            imshow("img_server",result);
            free_image(im);
            free_image(sized);
            if(waitKey(30)==27) break;
            count = 0;
        }
        
    }

    close(client_socket);
    close(server_socket);

}

}
//#endif
