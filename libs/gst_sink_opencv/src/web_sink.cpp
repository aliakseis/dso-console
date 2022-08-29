#include "web_sink.h"

#include "echoclient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QEventLoop>

#include <QJsonDocument>

#include <QSettings>

#include <opencv2/imgcodecs.hpp>


static QString getWebSocketDebuggerUrl()
{
    QNetworkAccessManager    networkAccessManager;
    QNetworkRequest request(QStringLiteral("http://localhost:9222/json/list"));

    QNetworkReply* reply = networkAccessManager.get(request);

    QEventLoop requrestWaitLoop;

    int redirectCount = 0;
    enum { REDIRECT_LIMIT = 10 };

    QByteArray syncResult;

    std::function<void()> finishedLam;
    finishedLam = [&reply, &redirectCount, &networkAccessManager, &finishedLam, &requrestWaitLoop, &syncResult] {
        QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        if (!possibleRedirectUrl.isNull() && redirectCount++ < REDIRECT_LIMIT)
        {
            QUrl redirect = possibleRedirectUrl.toUrl();
            QUrl url = reply->url();
            reply->deleteLater();
            reply = networkAccessManager.get(QNetworkRequest(
                redirect.isRelative() ? url.scheme() + "://" + url.host() + redirect.toString() : redirect));

            QObject::connect(reply, &QNetworkReply::finished, finishedLam);
            //QObject::connect(reply, &QNetworkReply::NetworkError,
            //               reply, SLOT(onNetworkError(QNetworkReply::NetworkError))));
            return;
        }

        syncResult = reply->readAll();
        requrestWaitLoop.quit();
        reply->deleteLater();
        reply = nullptr;
    };

    QObject::connect(reply, &QNetworkReply::finished, finishedLam);

    requrestWaitLoop.exec();

    qDebug() << syncResult;

    auto doc = QJsonDocument::fromJson(syncResult);

    return doc[0]["webSocketDebuggerUrl"].toString();
}


bool WebThread::init()
{
    moveToThread(this);

    m_webSocketDebuggerUrl = getWebSocketDebuggerUrl();

    if (m_webSocketDebuggerUrl.isEmpty())
    {
        const auto split = QSettings(R"(HKEY_CLASSES_ROOT\ChromeHTML\shell\open\command)", QSettings::NativeFormat).value(".")
            .toString().split('"');

        if (split.length() < 2) {
            return false;
        }

        const auto path = split[1];

        m_chromeProces.start(path, { "--remote-debugging-port=9222", "--headless" });
        if (!m_chromeProces.waitForStarted(5000))
        {
            // TODO Camera error message
            qDebug() << "Timed out starting a child process";
            return false;
        }

        m_webSocketDebuggerUrl = getWebSocketDebuggerUrl();
    }

    return true;
}

void WebThread::run()
{
    m_echoClient = new EchoClient(m_webSocketDebuggerUrl, m_url, m_width, m_height, false, this);
    m_echoClient->moveToThread(this);
    QObject::connect(m_echoClient, &EchoClient::closed, this, &WebThread::quit);
    QObject::connect(m_echoClient, &EchoClient::dataReceived, this, &WebThread::onDataReceived);

    exec();

    emit cameraDisconnected(m_interruptRequested);
}

void WebThread::onDataReceived(const QByteArray& data)
{
    if (m_queueSize < MAX_QUEUE_SIZE)
    {
        ++m_queueSize;
        auto mat = cv::imdecode({ data.data(), data.size() }, cv::IMREAD_COLOR);
        emit newImage(mat);
    }
}
