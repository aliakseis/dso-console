#include "VideoSaver.h"

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

#include <QDebug>

VideoSaver::VideoSaver() = default;
VideoSaver::~VideoSaver() = default;

void VideoSaver::onNewImage(const cv::Mat& frame, QString savePath, int sliceSeconds)
{
    QDateTime now = QDateTime::currentDateTime();
    if (!m_videoWriter || m_startTime.isNull() || m_startTime.msecsTo(now) >= sliceSeconds * 1000)
    {
        m_startTime = now;
        const auto name = now.toString("yyMMddhhmmss");
        const auto path = savePath + '/' + name + ".avi";// ".mp4";
        const auto fps = 30;
        const auto frame_width = frame.cols;
        const auto frame_height = frame.rows;
        m_videoWriter = std::make_unique<cv::VideoWriter>(
            path.toStdString(),
            cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),//'m', 'p', '4', 'v'), 
            fps,
            cv::Size(frame_width, frame_height));
    }
    
    m_videoWriter->write(frame);
}

void VideoSaver::onVideoStopped()
{
    qDebug() << __FUNCTION__;

    m_videoWriter.reset();
}
