#include <QDebug>

#include "weechatmessage.h"

QDebug operator<<(QDebug d, const WeeChatMessage &m)
{
    d << "id:" << m.id();
    d << "length:" << m.length();
    d << "object:" << m.object();
    return d;
}

