#pragma once

//#include <opencv2/core/mat.hpp>
#include <QString>
#include <QDateTime>

#include <memory>

namespace cv {
 
class Mat;
class VideoWriter;

}


class VideoSaver
{
public:
    VideoSaver();
    ~VideoSaver();
    VideoSaver(const VideoSaver&) = delete;
    VideoSaver& operator =(const VideoSaver&) = delete;

    void onNewImage(const cv::Mat& frame, QString savePath, int sliceSeconds);
    void onVideoStopped();

private:
    std::unique_ptr<cv::VideoWriter> m_videoWriter;
    QDateTime m_startTime;
};
