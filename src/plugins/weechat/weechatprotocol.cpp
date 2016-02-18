/*
  Copyright (C) 2013-2014 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QLoggingCategory>

#include "weechatprotocol.h"
#include "weechatbuffer.h"

IRC_USE_NAMESPACE

Q_LOGGING_CATEGORY(lc, "protocol.weechat")

WeeChatProtocol::WeeChatProtocol(IrcConnection* connection) : IrcProtocol(connection)
{
    qCDebug(lc) << "Created WeeChat protocol instance" << this;
    connect(&m_socket, &QWebSocket::binaryMessageReceived, this, &WeeChatProtocol::processMessage);
    connect(&m_socket, &QWebSocket::connected, this, [this]() {
        qCDebug(lc) << "Websocket connected" << this;
        receiveInfo(Irc::RPL_MYINFO, "Websocket connection established");
        sendCommand("init password=undefined");
        sendCommand("info version");
        sendCommand("hdata buffer:gui_buffers(*) number,full_name,short_name,type,nicklist,title,local_variables");
        sendCommand("hdata buffer:gui_buffers(*)/own_lines/last_line(-120)/data date,displayed,prefix,message");
        sendCommand("sync");
    });
    connect(&m_socket, &QWebSocket::disconnected, this, [this]() {
        qCDebug(lc) << "Websocket disconnected" << this;
        receiveError(tr("Websocket disconnected"));
    });
    typedef void (QWebSocket::* ErrorSignal)(QAbstractSocket::SocketError);
    connect(&m_socket, static_cast<ErrorSignal>(&QWebSocket::error), this, [this]() {
        qCDebug(lc) << "Websocket error" << this << m_socket.errorString();
        receiveError(tr("Websocket error: %1").arg(m_socket.errorString()));
    });
}

WeeChatProtocol::~WeeChatProtocol()
{
    qCDebug(lc) << "Destroyed WeeChat protocol instance" << this;
}

void WeeChatProtocol::sendCommand(const QByteArray &command)
{
    QByteArray buf = QString::fromLatin1("(%1) %2\n").arg(m_nextMessageId++).arg(QString::fromUtf8(command)).toUtf8();
    qDebug() << "<< " << buf;
    m_socket.sendBinaryMessage(buf);

}

void WeeChatProtocol::open()
{
    qCDebug(lc) << "Opening WeeChat protocol instance" << this;
    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        qCWarning(lc) << "Already connected";
        return;
    }

    setStatus(IrcConnection::Connecting);
    m_socket.open(QUrl("ws://localhost:9001/weechat"));
    m_nextMessageId = 0;
}

void WeeChatProtocol::close()
{
    qCDebug(lc) << "Closing WeeChat protocol instance" << this;
    m_socket.close();
}

void WeeChatProtocol::read()
{
    qCDebug(lc) << "Reading WeeChat protocol instance" << this;
}

bool WeeChatProtocol::write(const QByteArray &data)
{
    qCDebug(lc) << "Writing WeeChat protocol instance" << this << data;
    return true;
}

// General idea:
// We have data in QBA.
// We manage this data inside a WeeChatBuffer instance, which is responsible for
// acting as a raw repository of bytes, plus the logic to split that out.
//
// From there, we read WeeChatMessage instances.
// A message has a length, id, type, and an object: WeeChatObject.
// A WeeChatObject has a type, and type-specific data in QVariant:
// strings have data, buffers have data, HDA is a QVariantHash, ...
void WeeChatProtocol::processMessage(const QByteArray& message)
{
    qCDebug(lc) << "Processing message " << this << message.toHex();

    WeeChatBuffer buf(message);

    while (buf.hasMessage()) {
        WeeChatMessage msg = buf.nextMessage();

        if (msg.id() == "1") {
            // we requested the node version, now we have it.
            if (msg.object().type() == WeeChatObjectType::ObjectTypeInfo) {
                QStringList data = msg.object().data().value<QStringList>();

                if (data.at(0) == "version") {
                    // version information which we requested on connect
                    receiveInfo(Irc::RPL_MYINFO, tr("Connected to core: %1").arg(data.at(1)));
                    setStatus(IrcConnection::Connected);

                    QHash<QString, QString> info;
                    info.insert("NETWORK", d.network->support("NETWORK"));
                    info.insert("PREFIX", d.network->support("PREFIX"));
                    info.insert("CHANTYPES", d.network->support("CHANTYPES"));
                    // TODO: ...
                    setInfo(info);
                }
            } else {
                // what.
                m_socket.close();
                return;
            }
        } else if (msg.id() == "2") {
            // buffer information
            receiveInfo(Irc::RPL_MYINFO, "Got buffer information: " + msg.id());
            receiveInfo(Irc::RPL_MYINFO, msg.object().data().toString());
        } else if (msg.id() == "3") {
            receiveInfo(Irc::RPL_MYINFO, "Got buffer contents: " + msg.id());
        } else {
            receiveInfo(Irc::RPL_MYINFO, "Got a message: " + msg.id());
        }
    }
}

QString WeeChatProtocol::prefix() const
{
    return connection()->nickName() + "!" + connection()->userName() + "@weechat";
}

void WeeChatProtocol::receiveInfo(int code, const QString &info)
{
    IrcMessage* msg = IrcMessage::fromParameters(prefix(), QString::number(code), QStringList() << connection()->nickName() << info, connection());
    IrcProtocol::receiveMessage(msg);
}

void WeeChatProtocol::receiveError(const QString& error)
{
    receiveInfo(Irc::ERR_UNKNOWNERROR, error);
    m_socket.close();
    setStatus(IrcConnection::Error);
}
