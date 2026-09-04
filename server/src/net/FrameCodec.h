#pragma once

// ============================================================================
// Framing 编解码器：处理 4 字节大端长度头 + payload 的帧格式
//
// 职责：
// - encode：给 payload 添加长度头，生成完整帧
// - decode：从缓冲区中提取完整帧，处理粘包/半包
// ============================================================================

#include <cstring>

#include <QByteArray>
#include <QtEndian>
#include <QtGlobal>

#include "protocol/Protocol.h"

namespace net {

// 编码结果
enum EncodeResult {
    EncodeOk = 0,
    EncodePayloadTooLarge = -1,
};

// 解码结果
enum DecodeResult {
    DecodeOk = 0,               // 成功解码一帧
    DecodeNeedMoreData = 1,     // 数据不足，需要继续接收
    DecodePayloadTooLarge = -1, // 声明的长度超过上限
    DecodeBadLength = -2,       // 长度字段非法（负数等）
};

class FrameCodec {
public:
    // 编码：payload → 4字节大端长度头 + payload
    static EncodeResult encode(const QByteArray& payload, QByteArray& outFrame)
    {
        if (payload.size() > protocol::kMaxPayloadBytes) {
            return EncodePayloadTooLarge;
        }

        outFrame.clear();
        outFrame.reserve(protocol::kFrameLengthPrefixBytes + payload.size());

        // 写入大端长度头
        quint32 payloadLength = qToBigEndian<quint32>(static_cast<quint32>(payload.size()));
        outFrame.append(reinterpret_cast<const char*>(&payloadLength),
                        protocol::kFrameLengthPrefixBytes);
        outFrame.append(payload);

        return EncodeOk;
    }

    // 解码：从缓冲区中尝试提取一帧
    // 输入：buffer（可能包含半包、一包、多包）
    // 输出：outPayload（单帧 payload）、consumedBytes（本次消费的字节数）
    static DecodeResult decode(const QByteArray& buffer,
                               QByteArray& outPayload,
                               int& consumedBytes)
    {
        consumedBytes = 0;

        // 长度头都不够
        if (buffer.size() < protocol::kFrameLengthPrefixBytes) {
            return DecodeNeedMoreData;
        }

        // 读取大端长度头
        quint32 payloadLength = 0;
        std::memcpy(&payloadLength, buffer.constData(), protocol::kFrameLengthPrefixBytes);
        payloadLength = qFromBigEndian<quint32>(payloadLength);

        // 防御：拒绝超长 payload
        if (payloadLength > protocol::kMaxPayloadBytes) {
            return DecodePayloadTooLarge;
        }

        // 计算完整帧大小
        const qint64 frameSize = protocol::kFrameLengthPrefixBytes +
                                 static_cast<qint64>(payloadLength);

        // 数据还不够一帧
        if (buffer.size() < frameSize) {
            return DecodeNeedMoreData;
        }

        // 提取 payload
        outPayload = buffer.mid(protocol::kFrameLengthPrefixBytes,
                                static_cast<int>(payloadLength));
        consumedBytes = static_cast<int>(frameSize);

        return DecodeOk;
    }
};

} // namespace net
