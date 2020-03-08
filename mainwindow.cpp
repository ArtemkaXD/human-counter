#include <QFileDialog>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define MIN_AREA 14000 //Минимальный размер контура

using namespace std;
using namespace cv;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    close = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    bool trap,fix;
    int dX,cX,inCount,outCount;
    Mat frame,firstFrame,grayFrame,dFrame;
    dX = cX = inCount = outCount = 0;
    trap = fix = false;
    VideoWriter video;

    //Задаю путь к ввидео файлу
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Video File"),
                                                    QDir::currentPath(),
                                                    tr("Images (*.avi *.mp4 *.mpg)"));
    string inString = fileName.toStdString();
    VideoCapture cap(inString);
    if(!cap.isOpened())
        cerr << "Error opening video file\n";
    else
    {
        int frame_width = cap.get(CAP_PROP_FRAME_WIDTH);
        int frame_height = cap.get(CAP_PROP_FRAME_HEIGHT);
        int ex = static_cast<int>(cap.get(CAP_PROP_FOURCC));
        fileName.chop(4);
        fileName.append(QString("_count.mp4"));
        inString = fileName.toStdString();
        video.open(inString,ex,cap.get(CAP_PROP_FPS), Size(frame_width,frame_height));
    }




    while(1)
    {
        //Получаю следующий кадр из видео потока
        cap >> frame;
        if (frame.empty())
          break;
        //Делаю текущий кадр чернобелым и применею размытие
        cvtColor(frame,grayFrame, COLOR_BGR2GRAY);
        GaussianBlur(grayFrame,grayFrame,Size(31, 31), 0);


        //Создаю первый опорный кадр
        if (firstFrame.empty())
        {
            firstFrame = grayFrame.clone();
            continue;
        }

        //Создаю однобитный кадр на основе разности текущего и первого кадра
        absdiff(firstFrame, grayFrame,dFrame);
        threshold(dFrame,dFrame, 52, 255, THRESH_BINARY);
        Mat element = getStructuringElement(MORPH_CROSS,Size (35, 27));
        dilate(dFrame,dFrame, element,Point(-1,-1),2);

        //Нахожу контуры объектов и задаю их центральную точку
        vector<vector<Point> > contours;
        findContours(dFrame.clone(),contours, RETR_EXTERNAL,CHAIN_APPROX_SIMPLE);
        vector<vector<Point> >::iterator contour = contours.begin();
        for(; contour != contours.end(); ++contour)
        {
            if (contourArea(*contour) < MIN_AREA)
                        continue;

            Moments M = moments(*contour);
            cX = M.m10/M.m00;

        }


        //Определяю направление смещение точек в центрально области кадра
        if ((200 < cX) && (cX< 300) && !trap && !fix)
        {
                dX = cX;
                trap = true;
        }
        else if ((200 < cX) && (cX< 300) && trap && !fix)
        {
            trap = false;
            if (cX > dX)
            {
                fix = true;
                inCount++;
            }
            else if (cX < dX)
            {
                fix = true;
                outCount++;
            }
        }
        else if ((cX < 200) || (cX > 300))
            fix = false;

        //Вывожу состояния счетчиков
        char str[32];
        sprintf(str,"OUT: %i",outCount);
        putText(frame,str,Point2i(10, 20),
                FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);
        sprintf(str,"IN: %i",inCount);
        putText(frame,str,Point2i(420, 20),
                FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);
        putText(frame,"Press ECS to exit",Point2i(180, 20),
                FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 1);


        //Вывожу кадр в окно
        namedWindow("Playback", WINDOW_GUI_NORMAL|WINDOW_AUTOSIZE);
        imshow( "Playback", frame );
        video.write(frame);


        char c=(char)waitKey(23);
        if((c==27) || close)
          break;
      }

    cap.release();
    video.release();
    destroyAllWindows();
    QApplication::quit();

}

void MainWindow::closeEvent (QCloseEvent *event)
{
   close = true;
}
