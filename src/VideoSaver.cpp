#include "VideoSaver.h"

#include "ffmpeg_encode.h"

#include <opencv2/opencv.hpp>

#include <QDebug>

namespace {

size_t GetSize(const cv::Mat& mat) { return mat.total(); }

void RunnerFunc(std::string filename, FfmpegEncoder::Params params,
    VideoSaver::MatQueuePtr matQueue, VideoSaver::StopWatcher stopWatcher)
{
    stopWatcher;
    FfmpegEncoder ffmpegEncoder(filename.c_str(), params);
    cv::Mat packet;
    while (matQueue->pop(packet) && packet.cols == params.width && packet.rows == params.height)
    {
        ffmpegEncoder.Write(packet.data, packet.step[0]);
    }
}


}


VideoSaver::VideoSaver()
    : m_stopped(new std::promise<void>,
        [](std::promise<void>* p) {
            qDebug() << __FUNCTION__ << "VideoSaver threads stopped.";
            p->set_value(); delete p;
        })
{
}

VideoSaver::~VideoSaver()
{
    sendStop();
    m_queue.reset();

    auto fut = m_stopped->get_future();
    qDebug() << __FUNCTION__ << "Stopping VideoSaver threads...";
    m_stopped.reset();
    fut.get();
}

void VideoSaver::onNewImage(const cv::Mat& frame, QString savePath, double fps, int sliceSeconds)
{
    QDateTime now = QDateTime::currentDateTime();
    if (!m_queue || m_startTime.isNull() || m_startTime.msecsTo(now) >= sliceSeconds * 1000)
    {
        m_startTime = now;
        const auto name = now.toString("yyMMddhhmmss");
        const auto path = savePath + '/' + name + ".mp4";
        const auto frame_width = frame.cols;
        const auto frame_height = frame.rows;

        FfmpegEncoder::Params params{
            frame_width,
            frame_height,
            fps,
            3LL * frame_width * frame_height,
            "veryfast",
            27,
            AV_PIX_FMT_BGR24,
            AV_PIX_FMT_YUV420P
        };

        sendStop();
        m_queue = std::make_shared<MatQueue>();
        // https://stackoverflow.com/a/23454840/10472202
        std::thread(RunnerFunc, path.toStdString(), params, m_queue, m_stopped).detach();
    }
    
    m_queue->push(frame);
}

void VideoSaver::onVideoStopped()
{
    qDebug() << __FUNCTION__;

    sendStop();
    m_queue.reset();
}

void VideoSaver::sendStop()
{
    if (m_queue)
    {
        cv::Mat frame;
        m_queue->push(frame);
    }
}
