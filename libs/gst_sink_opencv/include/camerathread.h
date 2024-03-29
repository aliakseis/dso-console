#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include "camerathreadbase.h"

#include <QMutex>
#include <QQueue>

#include <opencv2/core/core.hpp>
#include "gst_sink_opencv.hpp"

class CameraThread : public CameraThreadBase
{
    Q_OBJECT

public:
    CameraThread(QString pipelineStr, double fps);
    ~CameraThread();

    double getBufPerc() override;
    void dataConsumed() override {}
    cv::Size getSize() override { return {}; }
    std::pair<int, int> getFps() override { return {}; }

signals:
    void newImage( cv::Mat frame );
    void cameraDisconnected(bool ok, const QString& error);
    void cameraConnected();

protected:
    void run() override;

private:
    GstSinkOpenCV* mImageSink;
    QString mPipelineStr;
    double mFps;
};

#endif // CAMERATHREAD_H
