#ifndef PROTOCOL_HELPER_H
#define PROTOCOL_HELPER_H

#include "Protocol.h"

namespace protocol {

// 响应外壳的统一构造器（从 Service 层返回的 QJsonObject）
inline QJsonObject buildResponse(const QString& requestId, const QJsonObject& serviceResponse)
{
    int code = serviceResponse.value("code").toInt(CodeOk);
    QString message = serviceResponse.value("message").toString("ok");
    QJsonObject data = serviceResponse.value("data").toObject();

    return buildResponse(requestId, code, message, data);
}

// Service 层返回值辅助函数
inline QJsonObject makeSuccessResponse(const QJsonObject& data = QJsonObject())
{
    QJsonObject response;
    response["code"] = CodeOk;
    response["message"] = "ok";
    response["data"] = data;
    return response;
}

inline QJsonObject makeErrorResponse(int code, const QString& message)
{
    QJsonObject response;
    response["code"] = code;
    response["message"] = message;
    response["data"] = QJsonObject();
    return response;
}

} // namespace protocol

#endif // PROTOCOL_HELPER_H
