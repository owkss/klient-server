#ifndef FILEINFO_H
#define FILEINFO_H

#include <QDataStream>

#pragma pack(push, 1)
typedef struct FileInfo
{
    QString fName;
    QString fExt;
    char FHash[20];
    quint64 FSize{0};

    friend QDataStream &operator<<(QDataStream &out, const FileInfo &fi);
    friend QDataStream &operator>>(QDataStream &in, FileInfo &fi);
} FInfo;
#pragma pack(pop)

inline QDataStream &operator<<(QDataStream &in, const FileInfo &fi)
{
    in << fi.fName;
    in << fi.fExt;
    in.writeRawData(fi.FHash, sizeof(fi.FHash));
    in << fi.FSize;
    return in;
}

inline QDataStream &operator>>(QDataStream &out, FileInfo &fi)
{
    out >> fi.fName;
    out >> fi.fExt;
    out.readRawData(fi.FHash, sizeof(fi.FHash));
    out >> fi.FSize;
    return out;
}

#endif // FILEINFO_H
