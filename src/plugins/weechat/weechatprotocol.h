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

#ifndef WEECHATPROTOCOL_H
#define WEECHATPROTOCOL_H

#include <QWebSocket>

#include <ircprotocol.h>

class WeeChatProtocol : public IRC_PREPEND_NAMESPACE(IrcProtocol)
{
    Q_OBJECT

public:
    explicit WeeChatProtocol(IRC_PREPEND_NAMESPACE(IrcConnection*) connection);
    virtual ~WeeChatProtocol();

    void open() override;
    void close() override;
    void read() override;
    bool write(const QByteArray& data) override;

private slots:
    void processMessage(const QByteArray& message);
    void sendCommand(const QByteArray& buf);

    void receiveInfo(int code, const QString &info);
    void receiveError(const QString& error);

private:
    QString prefix() const;

    QWebSocket m_socket;
    quint32 m_nextMessageId = 0;
};

#endif // WEECHATPROTOCOL_H
