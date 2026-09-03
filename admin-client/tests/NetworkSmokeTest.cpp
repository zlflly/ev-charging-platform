#include "net/NetworkClient.h"
#include "protocol/Protocol.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QTimer>

#include <cstdio>

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    NetworkClient client;
    constexpr int kRequestCount = 5;
    int completed = 0;
    int failed = 0;

    QObject::connect(&client, &NetworkClient::connected, &application, [&] {
        for (int index = 0; index < kRequestCount; ++index) {
            const QString expected = QStringLiteral("admin-ping-%1").arg(index);
            QJsonObject data;
            data.insert(QStringLiteral("echo"), expected);
            client.sendRequest(QString::fromUtf8(protocol::action::kPing), data,
                [&, expected, index](const protocol::Response& response) {
                    const bool ok = response.isOk()
                        && response.data.value(QStringLiteral("echo")).toString() == expected;
                    std::printf("[%s] request %d (%s)\n",
                                ok ? "ok" : "fail", index,
                                response.requestId.toUtf8().constData());
                    ok ? ++completed : ++failed;
                    if (completed + failed == kRequestCount) {
                        std::printf(failed == 0 ? "SMOKE TEST PASSED\n" : "SMOKE TEST FAILED\n");
                        QCoreApplication::exit(failed == 0 ? 0 : 1);
                    }
                });
        }
    });

    QTimer::singleShot(15000, &application, [] {
        std::printf("SMOKE TEST FAILED (timeout)\n");
        QCoreApplication::exit(1);
    });

    client.connectToServer(QStringLiteral("127.0.0.1"), 9000);
    return application.exec();
}
