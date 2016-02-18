#ifndef WEECHATMESSAGE_H
#define WEECHATMESSAGE_H

#include <QByteArray>

#include "weechatobject.h"

class WeeChatMessage
{
public:
    void setLength(quint32 l) { m_length = l;}
    quint32 length() const { return m_length; }

    void setId(const QByteArray &id) { m_id = id; }
    QByteArray id() const { return m_id; }

    void setObject(const WeeChatObject &obj) { m_object = obj; }
    WeeChatObject object() const { return m_object; }

private:
    quint32 m_length = 0;
    QByteArray m_id;
    WeeChatObject m_object;
};

QDebug operator<<(QDebug d, const WeeChatMessage &m);

#endif // WEECHATMESSAGE_H
