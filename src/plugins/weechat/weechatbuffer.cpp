#include <QtEndian>
#include <QDebug>

#include "weechatbuffer.h"
#include "weechatmessage.h"

WeeChatBuffer::WeeChatBuffer(const QByteArray &data)
    : m_data(data)
{
}

bool WeeChatBuffer::hasMessage() const
{
    if (m_data.size() - m_pos < 4)
        return false;

    // don't use readUInt because we're effectively peeking
    quint32 sz = qFromBigEndian<quint32>((const uchar*)m_data.constData() + m_pos);
    if (m_data.size() - m_pos < sz)
        return false;

    return true;
}

WeeChatMessage WeeChatBuffer::nextMessage()
{
    bool ok = false;
    quint32 length = readInt(&ok);
    if (!ok)
        return WeeChatMessage();

    readChar(&ok); // ignore the compression byte
    if (!ok)
        return WeeChatMessage();

    QByteArray id = readString(&ok);
    if (!ok)
        return WeeChatMessage();

    // first up, the type
    QByteArray type = readType(&ok);
    if (!ok)
        return WeeChatMessage();

    WeeChatObject obj = readObject(type, &ok);
    if (!ok)
        return WeeChatMessage();

    WeeChatMessage message;
    message.setLength(length);
    message.setId(id);
    message.setObject(obj);

    qDebug() << obj;
    return message;
}

WeeChatObject WeeChatBuffer::readObject(const QByteArray &type, bool *ok)
{
    static int tabCount = 1;

    // compilers don't like goto over the top of variable initialization.
    bool ruok = false;
    WeeChatObject obj;
    WeeChatObjectType realType;
    QVariant realData;

    qDebug() << "Reading object " << type << "(recursion level" << tabCount++ << ")";

    if (type == "chr") {
        realType = WeeChatObjectType::ObjectTypeChar;
        char c = readChar(&ruok);
        if (!ruok)
            goto bad;
        realData = QVariant::fromValue(c);
    } else if (type == "int") {
        // 4 byte signed int
        realType = WeeChatObjectType::ObjectTypeInt;
        qint32 i = readInt(&ruok);
        if (!ruok)
            goto bad;
        realData = QVariant::fromValue(i);
    } else if (type == "lon") {
        // signed long integer, size is the first byte, the rest is base-10 encoded.
        realType = WeeChatObjectType::ObjectTypeLong;
        quint8 sz = readChar(&ruok);
        if (!ruok)
            goto bad;
        QByteArray ldata = readBytes(sz, &ruok);
        if (!ruok)
            goto bad;
        realData = QVariant::fromValue(ldata.toLongLong(0, 10));
    } else if (type == "str") {
        // 4 byte length, plus data (requires a \0 terminator).
        realType = WeeChatObjectType::ObjectTypeString;
        realData = readString(&ruok);
        if (!ruok)
            goto bad;
    } else if (type == "buf") {
        // 4 byte length, plus data
        realType = WeeChatObjectType::ObjectTypeBuffer;
        realData = readString(&ruok);
        if (!ruok)
            goto bad;
    } else if (type == "ptr") {
        // 1 byte length plus string
        realType = WeeChatObjectType::ObjectTypePointer;
        realData = readPointer(&ruok);
        if (!ruok)
            goto bad;
    } else if (type == "tim") {
        // 1 byte length plus string
        realType = WeeChatObjectType::ObjectTypeTime;
        quint8 sz = readChar(&ruok);
        if (!ruok)
            goto bad;
        QByteArray ldata = readBytes(sz, &ruok);
        if (!ruok)
            goto bad;
        realData = QVariant::fromValue(ldata.toInt(0, 10));
    } else if (type == "htb") {
        // hashtable
        realType = WeeChatObjectType::ObjectTypeHashtable;
        QByteArray kt = readType(&ruok);
        if (!ruok)
            goto bad;
        QByteArray vt = readType(&ruok);
        if (!ruok)
            goto bad;
        quint32 len = readUInt(&ruok);
        if (!ruok)
            goto bad;

        QVariantHash h;
        for (quint32 i = 0; i < len; i++) {
            WeeChatObject key = readObject(kt, &ruok);
            if (!ruok)
                goto bad;
            WeeChatObject value = readObject(vt, &ruok);
            if (!ruok)
                goto bad;

            // we need something string-like..
            Q_ASSERT(kt == "str" || kt == "buf");

            // we throw away the type information here, but that's probably ok.
            h[key.data().toString()] = value.data();
        }

        realData = h;
    } else if (type == "hda") {
        // hdata
        realType = WeeChatObjectType::ObjectTypeHdata;
        QByteArray path = readString(&ruok);
        int pointerCount = path.count('/') + 1;
        qDebug() << path << pointerCount;
        if (!ruok)
            goto bad;


        QByteArray keys = readString(&ruok);
        qDebug() << keys;
        if (!ruok)
            goto bad;

        QList<QByteArray> fields;
        QList<QByteArray> types;
        for (QByteArray ba : keys.split(',')) {
            QList<QByteArray> pair = ba.split(':');
            fields.push_back(pair[0]);
            types.push_back(pair[1]);
        }
        qDebug() << fields << types;

        quint32 count = readInt(&ruok);
        qDebug() << count;
        if (!ruok)
            goto bad;

        QVariantList ret;
        for (quint32 i = 0; i < count; i++) {
            QVariantList ptrs;
            for (int j = 0; j < pointerCount; j++) {
                quintptr ptr = readPointer(&ruok);
                if (!ruok)
                    goto bad;
                ptrs.push_back(QVariant(ptr));
            }
            QVariantHash elem;
            elem["communi_path"] = ptrs;
            for (int j = 0; j < fields.length(); j++) {
                WeeChatObject obj = readObject(types[j], &ruok);
                if (!ruok)
                    goto bad;
                elem[fields[j]] = obj.data(); // discarding type information, probably ok.
            }
            ret.append(elem);
        }

        qDebug() << "HDA done";
        realData = ret;
    } else if (type == "inf") {
        // info: name + content strings
        realType = WeeChatObjectType::ObjectTypeInfo;
        bool ruok2 = false;
        QByteArray key = readString(&ruok);
        QByteArray value = readString(&ruok2);

        if (!ruok || !ruok2)
            goto bad;

        QStringList list = { key, value };
        realData = QVariant(list);
    } else if (type == "inl") {
        // infolist content
        // name string
        // count int
        realType = WeeChatObjectType::ObjectTypeInfolist;

        // TODO
        Q_ASSERT(false);
    } else if (type == "arr") {
        // array of objects. 3 byte type of contents, number of objects,
        // objects.
        realType = WeeChatObjectType::ObjectTypeArray;
        QByteArray objTypes = readType(&ruok);
        if (!ruok)
            goto bad;
        quint32 len = readUInt(&ruok);
        if (!ruok)
            goto bad;
        QVariantList objs;
        for (quint32 i = 0; i < len; ++i) {
            WeeChatObject obj = readObject(objTypes, &ruok);
            if (!ruok)
                goto bad;

            // we throw away the type data here, but that's probably ok.
            objs.append(obj.data());
        }
        realData = QVariant::fromValue(objs);
    } else {
        goto bad;
    }

    obj.setType(realType);
    obj.setData(realData);

    qDebug() << "Read" << realData;
    tabCount--;
    return obj;
bad:
    qDebug() << "Error! " << realData << m_pos;
    if (ok)
        *ok = false;
    tabCount--;
    return WeeChatObject();
}

char WeeChatBuffer::readChar(bool *ok)
{
    if (m_data.size() - m_pos < 1) {
        if (ok)
            *ok = false;
        return 0;
    }

    if (ok)
        *ok = true;
    m_pos += 1;
    return *(m_data.constData() + m_pos - 1);
}

quint32 WeeChatBuffer::readUInt(bool *ok)
{
    if (m_data.size() - m_pos < 4) {
        if (ok)
            *ok = false;
        return 0;
    }

    if (ok)
        *ok = true;
    m_pos += 4;
    return qFromBigEndian<quint32>((const uchar*)m_data.constData() + m_pos - 4);
}

quint32 WeeChatBuffer::readInt(bool *ok)
{
    if (m_data.size() - m_pos < 4) {
        if (ok)
            *ok = false;
        return 0;
    }

    if (ok)
        *ok = true;
    m_pos += 4;
    return qFromBigEndian<qint32>((const uchar*)m_data.constData() + m_pos - 4);
}

QByteArray WeeChatBuffer::readString(bool *ok)
{
    bool lok;
    qint32 slen = readInt(&lok);

    if (slen == -1) {
        // "A NULL string (NULL pointer in C) has a length of -1"
        if (ok)
            *ok = true;
        return QByteArray();
    }

    // if the read fails, we didn't have enough bytes for a 4 byte length
    // prefix.
    if (!lok) {
        if (ok)
            *ok = false;
        return QByteArray();
    }

    // ignore malformed strings
    if (m_data.size() - m_pos < (quint32)slen) {
        if (ok)
            *ok = false;
        return QByteArray();
    }

    if (ok)
        *ok = true;
    m_pos += slen;
    return QByteArray(m_data.constData() + m_pos - slen, slen);
}

QByteArray WeeChatBuffer::readBytes(unsigned int len, bool *ok)
{
    if (m_data.size() - m_pos < len) {
        if (ok)
            *ok = false;
        return QByteArray();
    }

    if (ok)
        *ok = true;
    m_pos += len;
    return QByteArray(m_data.constData() + m_pos - len, len);
}

quintptr WeeChatBuffer::readPointer(bool *ok) {
    bool ruok = false;
    QByteArray pdata;

    quint8 len = readChar(&ruok);
    if (!ruok)
        goto bad;

    pdata = readBytes(len, &ruok);
    if (!ruok)
        goto bad;
    return pdata.toULongLong(0, 16);

bad:
    if (ok)
        *ok = false;
    return 0;
}

QByteArray WeeChatBuffer::readType(bool *ok)
{
    return readBytes(3, ok);
}

