#include "camerathread.h"

#include <QDebug>
#include <string>
#include <utility>

CameraThread::CameraThread(QString pipelineStr, double fps)
    : mPipelineStr(std::move(pipelineStr))
    , mImageSink(nullptr)
{
    mFps = fps;
}

CameraThread::~CameraThread()
{
    if( isRunning() )
    {
        requestInterruption();

        if( !wait( 5000 ) )
        {
            qDebug() << tr("CameraThread not stopped. Forcing exit!");
        }
    }
}


void CameraThread::run()
{
//#ifdef USE_ARM
//    std::string pipeline = "udpsrc name=videosrc port=5000 ! "
//                           "application/x-rtp,media=video,encoding-name=H264,pt=96 ! "
//                           "rtph264depay ! h264parse ! omxh264dec";
//#else
//    std::string pipeline = "udpsrc name=videosrc port=5000 ! "
//                           "application/x-rtp,media=video,encoding-name=H264,pt=96 ! "
//                           "rtph264depay ! h264parse ! avdec_h264";
//#endif

    auto [mImageSink, error] = GstSinkOpenCV::Create(mPipelineStr.toStdString(), 10, 5);

    if(!mImageSink)
    {
        emit cameraDisconnected(false, QString::fromStdString(error));
        return;
    }

    emit cameraConnected();

    for(;;)
    {
        if( isInterruptionRequested() )
        {
            qDebug() << tr("CameraThread interruption requested");

            break;
        }

        cv::Mat frame = mImageSink->getLastFrame();

        if( !frame.empty() && frame.rows!=0 && frame.cols!=0 )
        {
            emit newImage( frame );
            msleep( 500.0/mFps );
        }
        else
        {
            msleep( 300.0/mFps );
        }

    }

    delete mImageSink;

    qDebug() << tr("CameraThread stopped.");

    emit cameraDisconnected(true, {});
}

double CameraThread::getBufPerc()
{
    if( !mImageSink )
    {
        return 0.0;
    }

    return mImageSink->getBufPerc();
}
