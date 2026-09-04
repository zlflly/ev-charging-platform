// ============================================================================
// FrameCodec 单元测试：粘包 / 半包 / 超长包 / 空包 / 连续多帧
// ============================================================================

#include <cstring>

#include <QTest>
#include <QtEndian>

#include "net/FrameCodec.h"

using namespace net;

class FrameCodecTest : public QObject {
    Q_OBJECT

private slots:
    // 编码：长度头为大端，且不含自身 4 字节
    void encodeWritesBigEndianLengthPrefix()
    {
        QByteArray frame;
        QCOMPARE(FrameCodec::encode(QByteArray("hello"), frame), EncodeOk);
        QCOMPARE(frame.size(), qsizetype(4 + 5));

        quint32 length = 0;
        std::memcpy(&length, frame.constData(), 4);
        QCOMPARE(qFromBigEndian<quint32>(length), 5u);
        QCOMPARE(frame.mid(4), QByteArray("hello"));
    }

    // 编码：超过 16MB 上限的 payload 被拒绝
    void encodeRejectsOversizedPayload()
    {
        const QByteArray oversized(static_cast<qsizetype>(protocol::kMaxPayloadBytes) + 1, 'x');
        QByteArray frame;
        QCOMPARE(FrameCodec::encode(oversized, frame), EncodePayloadTooLarge);
    }

    // 解码：完整一帧
    void decodeReadsCompleteFrame()
    {
        QByteArray frame;
        FrameCodec::encode(QByteArray("world"), frame);

        QByteArray payload;
        int consumedBytes = 0;
        QCOMPARE(FrameCodec::decode(frame, payload, consumedBytes), DecodeOk);
        QCOMPARE(payload, QByteArray("world"));
        QCOMPARE(qsizetype(consumedBytes), frame.size());
    }

    // 半包：长度头都没收齐
    void decodeWaitsForIncompleteLengthPrefix()
    {
        QByteArray payload;
        int consumedBytes = 0;
        QCOMPARE(FrameCodec::decode(QByteArray(2, '\0'), payload, consumedBytes),
                 DecodeNeedMoreData);
        QCOMPARE(consumedBytes, 0);
    }

    // 半包：长度头齐了但 payload 不够
    void decodeWaitsForIncompletePayload()
    {
        QByteArray buffer;
        const quint32 declaredLength = qToBigEndian<quint32>(10);
        buffer.append(reinterpret_cast<const char*>(&declaredLength), 4);
        buffer.append("12345"); // 声明 10 字节，实到 5 字节

        QByteArray payload;
        int consumedBytes = 0;
        QCOMPARE(FrameCodec::decode(buffer, payload, consumedBytes), DecodeNeedMoreData);
        QCOMPARE(consumedBytes, 0);
    }

    // 粘包：一个缓冲区里两帧，逐帧取出
    void decodeSplitsGluedFrames()
    {
        QByteArray first;
        QByteArray second;
        FrameCodec::encode(QByteArray("first"), first);
        FrameCodec::encode(QByteArray("second"), second);
        QByteArray buffer = first + second;

        QByteArray payload;
        int consumedBytes = 0;

        QCOMPARE(FrameCodec::decode(buffer, payload, consumedBytes), DecodeOk);
        QCOMPARE(payload, QByteArray("first"));
        buffer.remove(0, consumedBytes);

        QCOMPARE(FrameCodec::decode(buffer, payload, consumedBytes), DecodeOk);
        QCOMPARE(payload, QByteArray("second"));
        buffer.remove(0, consumedBytes);

        QVERIFY(buffer.isEmpty());
    }

    // 恶意长度声明：超过上限直接判错，不预分配内存
    void decodeRejectsOversizedLengthPrefix()
    {
        QByteArray buffer;
        const quint32 hugeLength =
            qToBigEndian<quint32>(static_cast<quint32>(protocol::kMaxPayloadBytes) + 1);
        buffer.append(reinterpret_cast<const char*>(&hugeLength), 4);

        QByteArray payload;
        int consumedBytes = 0;
        QCOMPARE(FrameCodec::decode(buffer, payload, consumedBytes), DecodePayloadTooLarge);
    }

    // 空 payload：只有长度头，也是合法帧
    void encodeDecodeEmptyPayload()
    {
        QByteArray frame;
        QCOMPARE(FrameCodec::encode(QByteArray(), frame), EncodeOk);
        QCOMPARE(frame.size(), qsizetype(4));

        QByteArray payload;
        int consumedBytes = 0;
        QCOMPARE(FrameCodec::decode(frame, payload, consumedBytes), DecodeOk);
        QVERIFY(payload.isEmpty());
        QCOMPARE(consumedBytes, 4);
    }

    // 连续 100 帧灌进同一个缓冲区，顺序与内容都不能错
    void decodeHandlesHundredGluedFrames()
    {
        QByteArray buffer;
        for (int i = 0; i < 100; ++i) {
            QByteArray frame;
            QCOMPARE(FrameCodec::encode(QStringLiteral("frame-%1").arg(i).toUtf8(), frame),
                     EncodeOk);
            buffer.append(frame);
        }

        int decodedCount = 0;
        while (!buffer.isEmpty()) {
            QByteArray payload;
            int consumedBytes = 0;
            QCOMPARE(FrameCodec::decode(buffer, payload, consumedBytes), DecodeOk);
            QCOMPARE(payload, QStringLiteral("frame-%1").arg(decodedCount).toUtf8());
            buffer.remove(0, consumedBytes);
            ++decodedCount;
        }
        QCOMPARE(decodedCount, 100);
    }
};

QTEST_APPLESS_MAIN(FrameCodecTest)

#include "FrameCodecTest.moc"
