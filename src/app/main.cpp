/*
* Copyright (C) 2008-2013 The Communi Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "appwindow.h"
#include <QApplication>
#include <QNetworkProxy>
#include <QIcon>
#include <QUrl>
#include <Irc>

static void setApplicationProxy(QUrl url)
{
    if (!url.isEmpty()) {
        if (url.port() == -1)
            url.setPort(8080);
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, url.host(), url.port(), url.userName(), url.password());
        QNetworkProxy::setApplicationProxy(proxy);
    }
}

int main(int argc, char* argv[])
{
#ifdef Q_OS_MAC
    // QTBUG-32789 - GUI widgets use the wrong font on OS X Mavericks
    QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
#endif

    QApplication app(argc, argv);
    app.setApplicationName("CommuniNG");
    app.setOrganizationName("Communi");
    app.setApplicationVersion(Irc::version());
    app.setOrganizationDomain("communi.github.com");

    AppWindow window;
    QStringList args = app.arguments();

    QUrl proxy;
    int index = args.indexOf("-proxy");
    if (index != -1)
        proxy = QUrl(args.value(index + 1));
    else
        proxy = QUrl(qgetenv("http_proxy"));
    setApplicationProxy(proxy);

    window.show();
    return app.exec();
}
