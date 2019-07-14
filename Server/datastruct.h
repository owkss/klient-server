#ifndef DATASTRUCT_H
#define DATASTRUCT_H

#include <QDataStream>

#pragma pack(push, 1)
typedef struct Fields
{
    char literal;
    quint16 x{0};
    quint16 y{0};
    qreal length{0.0};

    friend QDataStream &operator<<(QDataStream &in, const Fields &f);
    friend QDataStream &operator>>(QDataStream &out, Fields &f);
} SData;
#pragma pack(pop)

inline QDataStream &operator<<(QDataStream &in, const Fields &f)
{
    in.writeRawData(&f.literal, sizeof(char));
    in << f.x;
    in << f.y;
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);
    in << f.length;

    return in;
}

inline QDataStream &operator>>(QDataStream &out, Fields &f)
{
    out.readRawData(&f.literal, sizeof(char));
    out >> f.x;
    out >> f.y;
    out.setFloatingPointPrecision(QDataStream::DoublePrecision);
    out >> f.length;

    return out;
}

#endif // DATASTRUCT_H
