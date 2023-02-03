#pragma once

#include "fqueue.h"

//#include <opencv2/core/mat.hpp>
#include <QString>
#include <QDateTime>

#include <memory>
#include <future>

namespace cv {
 
class Mat;

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

    typedef FQueue<cv::Mat, 15000000, 500> MatQueue;
    typedef std::shared_ptr<MatQueue> MatQueuePtr;
    typedef std::shared_ptr<std::promise<void>> StopWatcher;

private:
    QDateTime m_startTime;
    StopWatcher m_stopped;
    MatQueuePtr m_queue;
};
