/*
* Copyright (C) 2008-2013 Communi authors
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

#include "mainwindow.h"
#include "application.h"
#include "connectionwizard.h"
#include "multisessiontabwidget.h"
#include "sessiontreewidget.h"
#include "sessiontabwidget.h"
#include "sessiontreeitem.h"
#include "connectioninfo.h"
#include "addviewdialog.h"
#include "searchpopup.h"
#include "messageview.h"
#include "sessionitem.h"
#include "homepage.h"
#include "overlay.h"
#include "toolbar.h"
#include "session.h"
#include "qtdocktile.h"
#include <QSettings>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent),
    treeWidget(0), trayIcon(0), dockTile(0)
{
    tabWidget = new MultiSessionTabWidget(this);
    connect(tabWidget, SIGNAL(splitterChanged(QByteArray)), this, SLOT(splitterChanged(QByteArray)));

    HomePage* homePage = new HomePage(tabWidget);
    connect(homePage, SIGNAL(connectRequested()), this, SLOT(connectTo()));
    tabWidget->insertWidget(0, homePage);

    QSplitter* splitter = new QSplitter(this);
    splitter->setHandleWidth(1);
    splitter->addWidget(tabWidget);
    setCentralWidget(splitter);

    createTree();

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new TrayIcon(this);
        trayIcon->setIcon(QApplication::windowIcon());
        trayIcon->setVisible(true);
        connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    }

    if (QtDockTile::isAvailable())
        dockTile = new QtDockTile(this);

    QShortcut* shortcut = new QShortcut(QKeySequence::Quit, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));

    shortcut = new QShortcut(QKeySequence::New, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(connectTo()), Qt::QueuedConnection);

    shortcut = new QShortcut(QKeySequence::AddTab, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(addView()), Qt::QueuedConnection);

    shortcut = new QShortcut(QKeySequence::Close, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(closeView()));

    searchShortcut = new QShortcut(this);
    connect(searchShortcut, SIGNAL(activated()), this, SLOT(searchView()));

#ifdef Q_WS_MAC
    QMenu* menu = new QMenu(this);
    menuBar()->addMenu(menu);

    QAction* action = new QAction(tr("Connect"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(connectTo()));
    menu->addAction(action);

    action = new QAction(tr("Settings"), this);
    action->setMenuRole(QAction::PreferencesRole);
    connect(action, SIGNAL(triggered()), qApp, SLOT(showSettings()));
    menu->addAction(action);

    action = new QAction(tr("About %1").arg(Application::applicationName()), this);
    action->setMenuRole(QAction::AboutRole);
    connect(action, SIGNAL(triggered()), qApp, SLOT(aboutApplication()));
    menu->addAction(action);

    action = new QAction(tr("About Qt"), this);
    action->setMenuRole(QAction::AboutQtRole);
    connect(action, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    menu->addAction(action);
#endif // Q_WS_MAC

    QSettings settings;
    if (settings.contains("geometry"))
        restoreGeometry(settings.value("geometry").toByteArray());

    applySettings(Application::settings());
    connect(qApp, SIGNAL(settingsChanged(Settings)), this, SLOT(applySettings(Settings)));

    ConnectionInfos connections = settings.value("connections").value<ConnectionInfos>();
    foreach (const ConnectionInfo& connection, connections)
        connectToImpl(connection);

    if (connections.isEmpty())
        QTimer::singleShot(600, this, SLOT(initialize()));
}

MainWindow::~MainWindow()
{
}

QSize MainWindow::sizeHint() const
{
    return QSize(800, 600);
}

void MainWindow::connectTo(const QString& host, quint16 port, const QString& nick, const QString& password)
{
    ConnectionInfo conn;
    conn.host = host;
    conn.port = port;
    conn.nick = nick;
    conn.pass = password;
    connectTo(conn);
}

void MainWindow::connectTo(const ConnectionInfo& connection)
{
    ConnectionWizard wizard;
    wizard.setConnection(connection);

    if (!connection.host.isEmpty() && !connection.nick.isEmpty())
        connectToImpl(connection);
    else if (wizard.exec())
        connectToImpl(wizard.connection());
}

void MainWindow::connectToImpl(const ConnectionInfo& connection)
{
    Session* session = Session::fromConnection(connection, this);
    session->setEncoding(Application::encoding());
    if (!session->hasQuit() && session->ensureNetwork())
        session->open();
    tabWidget->addSession(session);

    connect(session, SIGNAL(activeChanged(bool)), this, SLOT(updateSession()));
    connect(session, SIGNAL(connectedChanged(bool)), this, SLOT(updateSession()));
    updateSession(session);

    SessionTabWidget* tab = tabWidget->sessionWidget(session);
    connect(tab, SIGNAL(viewAdded(MessageView*)), this, SLOT(viewAdded(MessageView*)));
    connect(tab, SIGNAL(viewRemoved(MessageView*)), this, SLOT(viewRemoved(MessageView*)));
    connect(tab, SIGNAL(viewRenamed(MessageView*)), this, SLOT(viewRenamed(MessageView*)));
    connect(tab, SIGNAL(viewActivated(MessageView*)), this, SLOT(viewActivated(MessageView*)));

    if (MessageView* view = tab->viewAt(0)) {
        treeWidget->addView(view->item());
        treeWidget->setCurrentView(view->item());
        treeWidget->parentWidget()->show();
        view->applySettings(Application::settings());
    }

    QSettings settings;
    if (settings.contains("list"))
        tab->restoreSplitter(settings.value("list").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("tree", treeWidget->saveState());
    settings.setValue("splitter", static_cast<QSplitter*>(centralWidget())->saveState());

    ConnectionInfos connections;
    QList<Session*> sessions = tabWidget->sessions();
    foreach (Session* session, sessions) {
        connections += session->toConnection();
        session->quit();
    }
    settings.setValue("connections", QVariant::fromValue(connections));

    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (trayIcon)
                trayIcon->unalert();
            if (dockTile)
                dockTile->setBadge(0);
        }
    }
}

void MainWindow::initialize()
{
    connectTo(ConnectionInfo());
}

void MainWindow::editSession(Session* session)
{
    ConnectionWizard wizard;
    wizard.setConnection(session->toConnection());
    if (wizard.exec())
        session->initFrom(wizard.connection());
}

void MainWindow::applySettings(const Settings& settings)
{
    searchShortcut->setKey(QKeySequence(settings.shortcuts.value(Settings::SearchView)));

    foreach (MessageView* view, findChildren<MessageView*>())
        view->applySettings(settings);
    treeWidget->applySettings(settings);
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::DoubleClick:
            setVisible(!isVisible());
            break;
        case QSystemTrayIcon::Trigger:
            raise();
            activateWindow();
            break;
        default:
            break;
    }
}

void MainWindow::highlighted(IrcMessage* message)
{
    Q_UNUSED(message);
    if (!isActiveWindow()) {
        QApplication::alert(this);
        if (trayIcon)
            trayIcon->alert();
        if (dockTile)
            dockTile->setBadge(dockTile->badge() + 1);
    }

    MessageView* view = qobject_cast<MessageView*>(sender());
    if (view) {
        SessionTreeItem* item = treeWidget->sessionItem(view->session());
        if (view->viewType() != MessageView::ServerView)
            item = item->findChild(view->item()->name());
        if (item) {
            item->setHighlighted(true);
            item->setBadge(item->badge() + 1);
        }
    }
}

void MainWindow::missed(IrcMessage* message)
{
    Q_UNUSED(message);
    MessageView* view = qobject_cast<MessageView*>(sender());
    if (view) {
        SessionTreeItem* item = treeWidget->sessionItem(view->session());
        if (view->viewType() != MessageView::ServerView)
            item = item->findChild(view->item()->name());
        if (item)
            item->setBadge(item->badge() + 1);
    }
}

void MainWindow::viewAdded(MessageView* view)
{
    connect(view, SIGNAL(highlighted(IrcMessage*)), this, SLOT(highlighted(IrcMessage*)));
    connect(view, SIGNAL(missed(IrcMessage*)), this, SLOT(missed(IrcMessage*)));

    view->applySettings(Application::settings());

    QSettings settings;
    if (settings.contains("list"))
        view->restoreSplitter(settings.value("list").toByteArray());

    Session* session = view->session();
    treeWidget->addView(view->item());
    if (settings.contains("tree"))
        treeWidget->restoreState(settings.value("tree").toByteArray());
    treeWidget->expandItem(treeWidget->sessionItem(session));
}

void MainWindow::viewRemoved(MessageView* view)
{
    treeWidget->removeView(view->item());
}

void MainWindow::viewRenamed(MessageView* view)
{
    treeWidget->removeView(view->item());
}

void MainWindow::viewActivated(MessageView* view)
{
    treeWidget->setCurrentView(view->item());
}

void MainWindow::closeTreeItem(SessionTreeItem* item)
{
    if (!item->parent()) {
        tabWidget->removeSession(item->session());
        treeWidget->parentWidget()->setVisible(!tabWidget->sessions().isEmpty());
    } else if (SessionTabWidget* tab = tabWidget->sessionWidget(item->session())) {
        tab->removeView(item->modelItem()->name());
    }
}

void MainWindow::currentTreeItemChanged(Session* session, const QString& view)
{
    SessionTabWidget* tab = tabWidget->sessionWidget(session);
    if (tab) {
        tabWidget->setCurrentWidget(tab);
        if (view.isEmpty())
            tab->setCurrentIndex(0);
        else
            tab->openView(view);
    }
    setWindowFilePath(view);
}

void MainWindow::splitterChanged(const QByteArray& state)
{
    QSettings settings;
    settings.setValue("list", state);
}

void MainWindow::updateSession(Session* session)
{
    if (!session)
        session = qobject_cast<Session*>(sender());
    SessionTabWidget* tab = tabWidget->sessionWidget(session);
    if (tab) {
        if (!tab->session()->isConnected()) {
            QObject* overlay = tab->property("_communi_overlay_").value<QObject*>();
            if (!overlay) {
                overlay = new Overlay(tab);
                tab->setProperty("_communi_overlay_", QVariant::fromValue(overlay));
                connect(overlay, SIGNAL(refresh()), tab->session(), SLOT(reconnect()));
            }
            overlay->setProperty("visible", true);
            overlay->setProperty("busy", tab->session()->isActive());
            overlay->setProperty("refresh", !tab->session()->isActive());
        } else {
            delete tab->property("_communi_overlay_").value<QObject*>();
            tab->setProperty("_communi_overlay_", QVariant());
        }
    }
}

void MainWindow::addView()
{
    SessionTabWidget* tab = tabWidget->currentWidget();
    if (tab && tab->session()->isActive()) {
        AddViewDialog dialog(tab->session(), this);
        if (dialog.exec()) {
            QString view = dialog.view();
            if (tab->session()->isChannel(view))
                tab->session()->sendCommand(IrcCommand::createJoin(view, dialog.password()));
            tab->openView(view);
        }
    }
}

void MainWindow::closeView()
{
    SessionTabWidget* tab = tabWidget->currentWidget();
    if (tab) {
        int index = tab->currentIndex();
        tab->closeView(index);
        if (index == 0) {
            tabWidget->removeSession(tab->session());
            treeWidget->parentWidget()->setVisible(!tabWidget->sessions().isEmpty());
        }
    }
}

void MainWindow::searchView()
{
    SearchPopup* search = new SearchPopup(this);
    connect(search, SIGNAL(searched(QString)), treeWidget, SLOT(search(QString)));
    connect(search, SIGNAL(searchedAgain(QString)), treeWidget, SLOT(searchAgain(QString)));
    search->popup();
}

void MainWindow::createTree()
{
    QSplitter* splitter = static_cast<QSplitter*>(centralWidget());
    QWidget* container = new QWidget(this);
    container->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    container->setVisible(false);

    treeWidget = new SessionTreeWidget(container);
    treeWidget->setFocusPolicy(Qt::NoFocus);

    connect(treeWidget, SIGNAL(editSession(Session*)), this, SLOT(editSession(Session*)));
    connect(treeWidget, SIGNAL(closeItem(SessionTreeItem*)), this, SLOT(closeTreeItem(SessionTreeItem*)));
    connect(treeWidget, SIGNAL(currentViewChanged(Session*, QString)), this, SLOT(currentTreeItemChanged(Session*, QString)));

    ToolBar* toolBar = new ToolBar(container);
    connect(toolBar, SIGNAL(aboutTriggered()), qApp, SLOT(aboutApplication()));
    connect(toolBar, SIGNAL(settingsTriggered()), qApp, SLOT(showSettings()));
    connect(toolBar, SIGNAL(connectTriggered()), this, SLOT(connectTo()));
    connect(toolBar, SIGNAL(joinTriggered()), this, SLOT(addView()));

    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->addWidget(treeWidget);
    layout->addWidget(toolBar);
    layout->setSpacing(0);
    layout->setMargin(0);

    container->setMinimumWidth(toolBar->sizeHint().width());
    splitter->insertWidget(0, container);
    splitter->setStretchFactor(1, 1);
    QSettings settings;
    if (settings.contains("splitter"))
        splitter->restoreState(settings.value("splitter").toByteArray());
}