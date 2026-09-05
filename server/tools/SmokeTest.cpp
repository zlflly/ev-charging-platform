// ============================================================================
// 冒烟测试客户端（Commit 0 验收项）
//
// 一次性灌 100 个 PING 请求，校验：
//   - 100 个响应全部收到，无丢帧
//   - 每个响应的 requestId 都能匹配到一个未完成的请求（framing 未串包）
//   - 所有响应 code == 0
//
// 用法：./ev-smoke-test [host] [port]
// ============================================================================

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTcpSocket>
#include <QTimer>

#include "net/FrameCodec.h"
#include "protocol/Protocol.h"

using namespace net;

namespace {
constexpr int kTotalRequests = 100;
constexpr int kTimeoutMs = 15 * 1000;
} // namespace

class SmokeTestClient : public QObject {
    Q_OBJECT

public:
    SmokeTestClient(QString host, quint16 port, QObject* parent = nullptr)
        : QObject(parent), m_host(std::move(host)), m_port(port)
    {
        m_socket = new QTcpSocket(this);
        connect(m_socket, &QTcpSocket::connected, this, &SmokeTestClient::onConnected);
        connect(m_socket, &QTcpSocket::readyRead, this, &SmokeTestClient::onReadyRead);
        connect(m_socket, &QTcpSocket::errorOccurred, this, &SmokeTestClient::onError);

        // 服务端卡住时不能无限等，超时即判失败。
        m_timeout.setSingleShot(true);
        m_timeout.setInterval(kTimeoutMs);
        connect(&m_timeout, &QTimer::timeout, this, &SmokeTestClient::onTimeout);
    }

    void start()
    {
        qInfo() << "[SmokeTest] connecting to" << m_host << m_port;
        m_socket->connectToHost(m_host, m_port);
        m_timeout.start();
    }

private slots:
    void onConnected()
    {
        qInfo() << "[SmokeTest] connected, sending" << kTotalRequests << "PING requests";
        m_elapsed.start();

        // 不等响应连续写入，让服务端一次 readyRead 收到多帧——这正是要验的粘包场景。
        for (int i = 0; i < kTotalRequests; ++i) {
            const QString requestId = QStringLiteral("smoke-%1").arg(i);
            const QJsonObject request = protocol::buildRequest(
                protocol::action::kPing, requestId, QJsonObject());

            QByteArray frame;
            if (FrameCodec::encode(QJsonDocument(request).toJson(QJsonDocument::Compact), frame)
                != EncodeOk) {
                fail(QStringLiteral("encode failed for %1").arg(requestId));
                return;
            }
            m_socket->write(frame);
            m_pendingRequestIds.insert(requestId);
        }
    }

    void onReadyRead()
    {
        m_receiveBuffer.append(m_socket->readAll());

        while (true) {
            QByteArray payload;
            int consumedBytes = 0;
            const DecodeResult result =
                FrameCodec::decode(m_receiveBuffer, payload, consumedBytes);

            if (result == DecodeNeedMoreData) {
                return;
            }
            if (result != DecodeOk) {
                fail(QStringLiteral("framing error %1").arg(result));
                return;
            }

            m_receiveBuffer.remove(0, consumedBytes);
            if (!handleResponse(payload)) {
                return;
            }

            if (m_pendingRequestIds.isEmpty()) {
                succeed();
                return;
            }
        }
    }

    void onError(QAbstractSocket::SocketError)
    {
        fail(QStringLiteral("socket error: ") + m_socket->errorString());
    }

    void onTimeout()
    {
        fail(QStringLiteral("timed out with %1 responses still pending")
                 .arg(m_pendingRequestIds.size()));
    }

private:
    bool handleResponse(const QByteArray& payload)
    {
        const QJsonDocument document = QJsonDocument::fromJson(payload);
        if (!document.isObject()) {
            fail(QStringLiteral("response is not a JSON object"));
            return false;
        }

        const QJsonObject response = document.object();
        const QString requestId = response.value(QStringLiteral("requestId")).toString();
        const int code = response.value(QStringLiteral("code")).toInt(protocol::CodeServerError);

        // remove() 返回 false 说明这个 requestId 从没发过或已回过一次，两种都是串包。
        if (!m_pendingRequestIds.remove(requestId)) {
            fail(QStringLiteral("unexpected or duplicated requestId: ") + requestId);
            return false;
        }
        if (code != protocol::CodeOk) {
            fail(QStringLiteral("%1 returned code %2: %3")
                     .arg(requestId)
                     .arg(code)
                     .arg(response.value(QStringLiteral("message")).toString()));
            return false;
        }
        return true;
    }

    void succeed()
    {
        const qint64 elapsedMs = m_elapsed.elapsed();
        qInfo() << "[SmokeTest]" << kTotalRequests << "responses matched in" << elapsedMs << "ms";
        qInfo() << "[SmokeTest] Test PASSED";
        QCoreApplication::exit(0);
    }

    void fail(const QString& reason)
    {
        qCritical() << "[SmokeTest] Test FAILED:" << reason;
        QCoreApplication::exit(1);
    }

    QString m_host;
    quint16 m_port;

    QTcpSocket* m_socket = nullptr;
    QByteArray m_receiveBuffer;
    QSet<QString> m_pendingRequestIds;
    QElapsedTimer m_elapsed;
    QTimer m_timeout;
};

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QString host = argc >= 2 ? QString::fromUtf8(argv[1])
                                   : QString::fromUtf8("127.0.0.1");
    const quint16 port = argc >= 3 ? static_cast<quint16>(QString::fromUtf8(argv[2]).toUShort())
                                   : protocol::kDefaultPort;

    SmokeTestClient client(host, port);
    client.start();

    return app.exec();
}

#include "SmokeTest.moc"
