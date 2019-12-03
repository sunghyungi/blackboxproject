#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->addItem(&pixmap);

    ui->redHisto->setScene(new QGraphicsScene(this));
    ui->redHisto->scene()->addItem(&pixmap2);

    ui->greenHisto->setScene(new QGraphicsScene(this));
    ui->greenHisto->scene()->addItem(&pixmap3);

    ui->blueHisto->setScene(new QGraphicsScene(this));
    ui->blueHisto->scene()->addItem(&pixmap4);

    ui->graphicsView_2->setScene(new QGraphicsScene(this));
    ui->graphicsView_2->scene()->addItem(&pixmapRGB);

    ui->graphicsView_3->setScene(new QGraphicsScene(this));
    ui->graphicsView_3->scene()->addItem(&pixmapDET);
}



MainWindow::~MainWindow()
{
    delete ui;
}

// 엣지 검출, 라인 그리기
void draw_houghLines(Mat image, Mat& dst, vector<Vec2f> lines, int nline)
{
    if (image.channels() == 3) image.copyTo(dst);
    else cvtColor(image, dst, COLOR_GRAY2RGB);

    for (int i = 0; i < min((int)lines.size(), nline); i++)
    {
        float rho = lines[i][0], theta = lines[i][1];
        double a = cos(theta), b = sin(theta);

        Point2d delta(1000 * -b, 1000 * a);
        Point2d pt(a*rho, b*rho);
        line(dst, pt + delta, pt - delta, Scalar(0, 255, 0), 1, LINE_AA);
    }
}




Mat Line_edge(Mat frame1)
{
                Mat lineframe;
                cvtColor(frame1, lineframe, COLOR_BGR2GRAY);
                double rho = 1, theta = CV_PI/180;
                Mat canny, dst1;
                GaussianBlur(lineframe, canny, Size(5,5), 2, 2);
                Canny(canny, canny, 100 ,150, 3);
                vector<Vec2f> lines1;
                HoughLines(canny, lines1, rho, theta, 50);
                draw_houghLines(canny, dst1, lines1, 10);

                return dst1;

}

// 히스토그램
void  calc_Histo(const Mat& image, Mat& hist, int bins, int range_max = 256)
{
    int		histSize[] = { bins };			// 히스토그램 계급개수
    float   range[] = { 0, (float)range_max };		// 히스토그램 범위
    int		channels[] = { 0 };				// 채널 목록
    const float* ranges[] = { range };

    calcHist(&image, 1, channels, Mat(), hist, 1, histSize, ranges);
}

Mat  make_palatte(int rows, int num)
{
    Mat hsv(rows, 1, CV_8UC3);
    for (int i = 0; i < rows; i++)
    {
//        uchar hue = saturate_cast<uchar>((float)i / rows * 180);
        if(num == 1){
            hsv.at<Vec3b>(i) = Vec3b(0, 0, 255);
        }
        else if(num == 2){
            hsv.at<Vec3b>(i) = Vec3b(0, 255, 0);
        }
        else if(num == 3){
            hsv.at<Vec3b>(i) = Vec3b(255, 0, 0);
        }
    }
    cvtColor(hsv, hsv, COLOR_BGR2RGB);
    return hsv;
}

void draw_histo_hue(Mat hist, Mat &hist_img, Size size = Size(256, 200), int a = 0)
{
    Mat hsv_palatte = make_palatte(hist.rows, a);

    hist_img = Mat(size, CV_8UC3, Scalar(255, 255, 255));
    float  bin = (float)hist_img.cols / hist.rows;
    normalize(hist, hist, 0, hist_img.rows, NORM_MINMAX);

    for (int i = 0; i<hist.rows; i++)
    {
        float start_x = (i * bin);
        float  end_x = (i + 1) * bin;
        Point2f pt1(start_x, 0);
        Point2f pt2(end_x, hist.at <float>(i));


        Scalar color = hsv_palatte.at<Vec3b>(i);				// 색상팔레트 색지정
        if (pt2.y>0) rectangle(hist_img, pt1, pt2, color, -1);	// 팔레트 색 그리기
    }
    flip(hist_img, hist_img, 0);
}

Mat histg(Mat frame, int num){

    Mat BGR_arr[3];
//    cvtColor(frame, HSV_img, COLOR_BGR2HSV);
    split(frame, BGR_arr);

    Mat hue_hist, hue_hist_img;
    if(num == 1){

        calc_Histo(BGR_arr[0], hue_hist, 256, 256);// Hue 채널 히스토그램 계산
        draw_histo_hue(hue_hist, hue_hist_img, Size(360, 200), 1); // 히스토그램 그래프
    }
    else if(num == 2)
    {

        calc_Histo(BGR_arr[1], hue_hist, 256, 256);// Hue 채널 히스토그램 계산
        draw_histo_hue(hue_hist, hue_hist_img, Size(360, 200), 2); // 히스토그램 그래프
    }
    else if(num == 3)
    {

        calc_Histo(BGR_arr[2], hue_hist, 256, 256);// Hue 채널 히스토그램 계산
        draw_histo_hue(hue_hist, hue_hist_img, Size(360, 200), 3); // 히스토그램 그래프
    }
    return hue_hist_img;
}


void MainWindow::on_startBtn_pressed()
{
    using namespace cv;

    if(video.isOpened())
    {
        ui->startBtn->setText("Start");
        video.release();
        return;
    }

    bool isCamera;
    int cameraIndex = ui->videoEdit->text().toInt(&isCamera);
    if(isCamera)
    {
        if(!video.open(cameraIndex))
        {
            QMessageBox::critical(this,
                                  "Camera Error",
                                  "Make sure you entered a correct camera index,"
                                  "<br>or that the camera is not being accessed by another program!");
            return;
        }
    }
    else
    {
        if(!video.open(ui->videoEdit->text().trimmed().toStdString()))
        {
            QMessageBox::critical(this,
                                  "Video Error",
                                  "Make sure you entered a correct and supported video file path,"
                                  "<br>or a correct RTSP feed URL!");
            return;
        }
    }

    ui->startBtn->setText("Stop");

    Mat frame1;

    video >> frame1;

    VideoWriter writer;
    double fps = 10;
    int fourcc = VideoWriter::fourcc('D', 'X', '5', '0');
    Size size(frame1.cols, frame1.rows);
    writer.open("../video_file.avi", fourcc, fps, size);
    int delay = cvRound(1000.0 / fps);



    while(video.isOpened())
    {
        video >> frame1;


        if(!frame1.empty())
        {

            if(ui->LineBtn->text()=="Normal")               // 엣지
            {
                frame1 = Line_edge(frame1);

            }

            cvtColor(frame1,frame1,COLOR_BGR2RGB);
            Mat red_hist, green_hist, blue_hist; // 히스토그램


            // TAB 1
            // ------------------------------------------------------------------------------------ TAB1

            if(ui->tabWidget->currentIndex()==0){

                QImage qimg(frame1.data,
                          frame1.cols,
                          frame1.rows,
                          frame1.step,
                          QImage::Format_RGB888);




                pixmap.setPixmap( QPixmap::fromImage(qimg));
                ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatio);

                // 히스토그램


                red_hist = histg(frame1, 1);
                green_hist = histg(frame1, 2);
                blue_hist = histg(frame1, 3);


                if(ui->recBtn->text()=="STOP")
                {

                    cvtColor(frame1,frame1, COLOR_RGB2BGR);
                    writer << frame1;
                    waitKey(delay);

                }

            }

            // ------------------------------------------------------------------------------------ TAB1

            // TAB 2
            // RGB COLOR split
            // ------------------------------------------------------------------------------------ TAB2
            if(ui->tabWidget->currentIndex()==1){
                Mat bgr[3];
                split(frame1,bgr);
                Mat frame2, ch_012;
                Mat zeros(bgr[0].size(),CV_8UC1,int(0));

                bool redbool = ui->RedCheck->checkState();
                bool greenbool = ui->GreenCheck->checkState();
                bool bluebool = ui->BlueCheck->checkState();


                // 3채널
                if(ui->chabtn->text() == "1channel")
                {
                    if(!redbool){
                        bgr[0] = zeros;
                    }
                    if(!greenbool){
                        bgr[1] = zeros;
                    }
                    if(!bluebool){
                        bgr[2] = zeros;
                    }

                    vector<Mat> vec_012;
                    vec_012.push_back(bgr[0]);
                    vec_012.push_back(bgr[1]);
                    vec_012.push_back(bgr[2]);
                    merge(vec_012, ch_012);
                    frame2 = ch_012;
                }
                else{       // 1채널
                    Mat chamat;


                    if(redbool && greenbool && bluebool){
                        chamat = (bgr[0]+bgr[1]+bgr[2])/3;
                    }
                    else if(redbool && greenbool){
                        chamat = (bgr[0]+bgr[1])/2;
                    }
                    else if(redbool && bluebool){
                        chamat = (bgr[0]+bgr[2])/2;
                    }
                    else if(greenbool && bluebool){
                        chamat = (bgr[1]+bgr[2])/2;
                    }
                    else if(redbool){
                        chamat = bgr[0];
                    }
                    else if(greenbool){
                        chamat = bgr[1];
                    }
                    else if(bluebool){
                        chamat = bgr[2];
                    }

                    cvtColor(chamat, frame2, COLOR_GRAY2RGB);
                }




                QImage qimgRGB(frame2.data,
                          frame2.cols,
                          frame2.rows,
                          frame2.step,
                          QImage::Format_RGB888);

                pixmapRGB.setPixmap( QPixmap::fromImage(qimgRGB));
                ui->graphicsView_2->fitInView(&pixmapRGB, Qt::KeepAspectRatio);








                // 히스토그램
                red_hist = histg(frame2, 1);
                green_hist = histg(frame2, 2);
                blue_hist = histg(frame2, 3);
            }


            // -------------------------------------------------------------------------------------- TAB2



            // TAB3 Detecting
            // -------------------------------------------------------------------------------------- TAB3
            if(ui->tabWidget->currentIndex()==2){
//                Mat frame3;
//                frame3 = frame1;

                if (ui->detectBtn->text() == "STOP"){
                    CascadeClassifier classifier("../~detectxml/cascade2.xml");

                    vector<Rect> car;
                    classifier.detectMultiScale(frame1, car);

                    for (Rect rc: car){
                        rectangle(frame1, rc, Scalar(255, 0, 255),2);
                    }

                    CascadeClassifier classifier2("../~detectxml/upperbody.xml");

                    vector<Rect> body;
                    classifier2.detectMultiScale(frame1, body);

                    for (Rect rc: body){
                        rectangle(frame1, rc, Scalar(255, 0, 0),2);
                    }

                }


                QImage qimgDetect(frame1.data,
                          frame1.cols,
                          frame1.rows,
                          frame1.step,
                          QImage::Format_RGB888);

                pixmapDET.setPixmap( QPixmap::fromImage(qimgDetect));
                ui->graphicsView_3->fitInView(&pixmapDET, Qt::KeepAspectRatio);

                red_hist = histg(frame1, 1);
                green_hist = histg(frame1, 2);
                blue_hist = histg(frame1, 3);

            }


            // -------------------------------------------------------------------------------------- TAB3


            // histogram draw
            // --------------------------------------------------------------------------------------------------


            QImage qimg2(red_hist.data,
                      red_hist.cols,
                      red_hist.rows,
                      red_hist.step,
                      QImage::Format_RGB888);

            QImage qimg3(green_hist.data,
                      green_hist.cols,
                      green_hist.rows,
                      green_hist.step,
                      QImage::Format_RGB888);

            QImage qimg4(blue_hist.data,
                      blue_hist.cols,
                      blue_hist.rows,
                      blue_hist.step,
                      QImage::Format_RGB888);


            pixmap2.setPixmap( QPixmap::fromImage(qimg2));
            ui->redHisto->fitInView(&pixmap2, Qt::KeepAspectRatio);
            pixmap3.setPixmap( QPixmap::fromImage(qimg3));
            ui->greenHisto->fitInView(&pixmap3, Qt::KeepAspectRatio);
            pixmap4.setPixmap( QPixmap::fromImage(qimg4));
            ui->blueHisto->fitInView(&pixmap4, Qt::KeepAspectRatio);

            // --------------------------------------------------------------------------------------------------

        }
        qApp->processEvents();
    }

    ui->startBtn->setText("Start");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(video.isOpened())
    {
        QMessageBox::warning(this,
                             "Warning",
                             "Stop the video before closing the application!");
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

void MainWindow::on_LineBtn_pressed()
{
    if(ui->LineBtn->text()=="Linesearch")
    {
        ui->LineBtn->setText("Normal");
    }
    else{
        ui->LineBtn->setText("Linesearch");
    }
}


// 위젯 이동 함수
#define I 4
void MainWindow::on_upButton_pressed()
{
    ui->graphicsView->move(ui->graphicsView->x(), ui->graphicsView->y()-I);
}

void MainWindow::on_leftButton_pressed()
{
    ui->graphicsView->move(ui->graphicsView->x()-I, ui->graphicsView->y());
}

void MainWindow::on_rightButton_pressed()
{
    ui->graphicsView->move(ui->graphicsView->x()+I, ui->graphicsView->y());
}

void MainWindow::on_downButton_pressed()
{
    ui->graphicsView->move(ui->graphicsView->x(), ui->graphicsView->y()+I);
}

// 위젯 확장, 축소 함수

void MainWindow::on_extendButton_pressed()
{
    ui->graphicsView->setFixedSize(ui->graphicsView->width()+2, ui->graphicsView->height()+3);
}

void MainWindow::on_reduceButton_pressed()
{
    ui->graphicsView->setFixedSize(ui->graphicsView->width()-2, ui->graphicsView->height()-3);
}

void MainWindow::on_clockWiseButton_pressed()
{
    ui->graphicsView->rotate(10);
}


void MainWindow::on_cclockWiseButton_pressed()
{
    ui->graphicsView->rotate(-10);
}


void MainWindow::on_recBtn_pressed()
{
    if(ui->recBtn->text()=="REC")
    {
        ui->recBtn->setText("STOP");
    }
    else{
        ui->recBtn->setText("REC");
    }
}

void MainWindow::on_detectBtn_pressed()
{
    if(ui->detectBtn->text()=="Detecting")
    {
        ui->detectBtn->setText("STOP");
    }
    else{
        ui->detectBtn->setText("Detecting");
    }
}

void MainWindow::on_chabtn_pressed()
{
    if(ui->chabtn->text() == "1channel")
    {
        ui->chabtn->setText("3channel");
    }
    else{
        ui->chabtn->setText("1channel");
    }
}
