#include "net/NetworkClient.h"
#include "protocol/Protocol.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QTimer>

#include <cstdio>

namespace {

void finish(bool ok, const char* message)
{
    std::printf("%s\n", message);
    QCoreApplication::exit(ok ? 0 : 1);
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    NetworkClient client;

    QObject::connect(&client, &NetworkClient::connected, &application, [&] {
        QJsonObject invalidData {
            {QStringLiteral("account"), QStringLiteral("admin")},
            {QStringLiteral("password"), QStringLiteral("wrong")},
        };
        client.sendRequest(QString::fromUtf8(protocol::action::kAdminLogin), invalidData,
            [&](const protocol::Response& invalidResponse) {
                if (invalidResponse.code != protocol::CodeInvalidAdminCredentials) {
                    finish(false, "AUTH SMOKE TEST FAILED (invalid credentials)");
                    return;
                }

                QJsonObject validData {
                    {QStringLiteral("account"), QStringLiteral("admin")},
                    {QStringLiteral("password"), QStringLiteral("123456")},
                };
                client.sendRequest(QString::fromUtf8(protocol::action::kAdminLogin), validData,
                    [](const protocol::Response& validResponse) {
                        const bool ok = validResponse.isOk()
                            && validResponse.data.value(QStringLiteral("adminId")).toDouble() > 0
                            && validResponse.data.value(QStringLiteral("account")).toString()
                                == QStringLiteral("admin");
                        finish(ok, ok ? "AUTH SMOKE TEST PASSED"
                                      : "AUTH SMOKE TEST FAILED (valid credentials)");
                    });
            });
    });

    QTimer::singleShot(15000, &application, [] {
        finish(false, "AUTH SMOKE TEST FAILED (timeout)");
    });

    client.connectToServer(QStringLiteral("127.0.0.1"), 8888);
    return application.exec();
}
