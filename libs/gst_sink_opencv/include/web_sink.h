#pragma once

#include "camerathreadbase.h"

#include <QProcess>

#include <atomic>

class EchoClient;

class WebThread : public CameraThreadBase
{
    enum { MAX_QUEUE_SIZE = 10 };

    Q_OBJECT

public:
    WebThread(const QString& url, int width, int height)
        : m_url(url), m_width(width), m_height(height)
    {
    }
    ~WebThread() = default;

    bool init();

    double getBufPerc() override { return m_queueSize / (double)MAX_QUEUE_SIZE; }
    void dataConsumed() override { --m_queueSize; }
    cv::Size getSize() override { return {}; }
    std::pair<int, int> getFps() override { return {}; }

    void requestInterruption() override
    {
        m_interruptRequested = true;
        quit();
    }

private slots:
    void onDataReceived(const QByteArray& data);

signals:
    void newImage(cv::Mat frame);
    void cameraDisconnected(bool ok, const QString& error);

protected:
    void run() override;

private:
    QString m_url;
    int m_width;
    int m_height;

    QString m_webSocketDebuggerUrl;

    QProcess m_chromeProces;
    EchoClient* m_echoClient{};

    bool m_interruptRequested = false;

    std::atomic_int m_queueSize{ 0 };
};
