#pragma once

#include "camerathreadbase.h"

#include <QProcess>

class EchoClient;

class WebThread : public CameraThreadBase
{
    Q_OBJECT

public:
    WebThread(const QString& url, int width, int height)
        : m_url(url), m_width(width), m_height(height)
    {
    }
    ~WebThread() = default;

    bool init();

    double getBufPerc() override { return 0; }
    void dataConsumed() override {}
    cv::Size getSize() override { return {}; }
    std::pair<int, int> getFps() override { return {}; }

    //void requestInterruption() override;

private slots:
    void onDataReceived(const QByteArray& data);

signals:
    void newImage(cv::Mat frame);
    void cameraDisconnected(bool ok);

protected:
    void run() override;

private:
    QString m_url;
    int m_width;
    int m_height;

    QString m_webSocketDebuggerUrl;

    QProcess m_chromeProces;
    EchoClient* m_echoClient{};
};
