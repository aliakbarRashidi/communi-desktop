#ifndef WEECHATOBJECT_H
#define WEECHATOBJECT_H

#include <QDebug>

enum class WeeChatObjectType
{
    ObjectTypeChar,
    ObjectTypeInt,
    ObjectTypeLong,
    ObjectTypeString,
    ObjectTypeBuffer,
    ObjectTypePointer,
    ObjectTypeTime,
    ObjectTypeHashtable,
    ObjectTypeHdata,
    ObjectTypeInfo,
    ObjectTypeInfolist,
    ObjectTypeArray
};

class WeeChatObject
{
public:
    void setData(const QVariant &data) { m_data = data; }
    QVariant data() const { return m_data; };

    void setType(WeeChatObjectType type) { m_type = type; }
    WeeChatObjectType type() const { return m_type; }
private:
    WeeChatObjectType m_type;
    QVariant m_data;
};

Q_DECLARE_METATYPE(WeeChatObject);

QDebug operator<<(QDebug d, const WeeChatObject &m);

#endif // WEECHATOBJECT_H
