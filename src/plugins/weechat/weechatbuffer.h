#ifndef WEECHATBUFFER_H
#define WEECHATBUFFER_H

#include <QByteArray>

#include "weechatmessage.h"

class WeeChatBuffer
{
public:
    WeeChatBuffer(const QByteArray &message);

    bool hasMessage() const;
    WeeChatMessage nextMessage();

private:
    WeeChatObject readObject(const QByteArray &type, bool *ok = 0);
    char readChar(bool *ok = 0);
    quint32 readUInt(bool *ok = 0);
    quint32 readInt(bool *ok = 0);
    QByteArray readString(bool *ok = 0);
    QByteArray readType(bool *ok = 0);
    QByteArray readBytes(unsigned int length, bool *ok = 0);
    quintptr readPointer(bool *ok = 0);

    QByteArray m_data;
    quint32 m_pos = 0;
};

#endif
