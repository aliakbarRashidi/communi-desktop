#include "weechatobject.h"

QDebug operator<<(QDebug d, const WeeChatObject &m)
{
    d << "type:" << int(m.type());
    d << "data:" << m.data();
    return d;
}

