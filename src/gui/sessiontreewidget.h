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

#ifndef SESSIONTREEWIDGET_H
#define SESSIONTREEWIDGET_H

#include <QTreeWidget>
#include <QShortcut>
#include <QColor>
#include <QHash>
#include "settings.h"

class Session;
struct Settings;
class SessionItem;
class MenuFactory;
class SessionModel;
class SessionTreeItem;

class SessionTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    SessionTreeWidget(QWidget* parent = 0);

    QSize sizeHint() const;

    QByteArray saveState() const;
    void restoreState(const QByteArray& state);

    MenuFactory* menuFactory() const;
    void setMenuFactory(MenuFactory* factory);

    enum ItemStatus { Active, Inactive, Highlight };

    QColor statusColor(ItemStatus status) const;
    void setStatusColor(ItemStatus status, const QColor& color);

    SessionTreeItem* treeItem(SessionItem* item) const;

public slots:
    void addView(SessionItem* view);
    void removeView(SessionItem* view);
    void renameView(SessionItem* view);
    void setCurrentView(SessionItem* view);

    void moveToNextItem();
    void moveToPrevItem();

    void moveToNextUnreadItem();
    void moveToPrevUnreadItem();

    void expandCurrentSession();
    void collapseCurrentSession();
    void moveToMostActiveItem();

    void search(const QString& search);
    void searchAgain(const QString& search);

    void applySettings(const Settings& settings);

signals:
    void editSession(Session* session);
    void closeItem(SessionTreeItem* item);
    void currentViewChanged(Session* session, const QString& view);

protected:
    void contextMenuEvent(QContextMenuEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    bool event(QEvent* event);
    QMimeData* mimeData(const QList<QTreeWidgetItem*> items) const;

private slots:
    void updateView(SessionItem* view = 0);
    void onItemExpanded(QTreeWidgetItem* item);
    void onItemCollapsed(QTreeWidgetItem* item);
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void delayedItemReset();
    void delayedItemResetTimeout();

private:
    void resetItem(SessionTreeItem* item);
    QTreeWidgetItem* lastItem() const;
    QTreeWidgetItem* nextItem(QTreeWidgetItem* from) const;
    QTreeWidgetItem* previousItem(QTreeWidgetItem* from) const;

    struct SessionTreeWidgetData {
        Settings settings;
        MenuFactory* menuFactory;
        QShortcut* prevShortcut;
        QShortcut* nextShortcut;
        QShortcut* prevUnreadShortcut;
        QShortcut* nextUnreadShortcut;
        QShortcut* expandShortcut;
        QShortcut* collapseShortcut;
        QShortcut* mostActiveShortcut;
        QHash<ItemStatus, QColor> colors;
        QSet<SessionTreeItem*> resetedItems;
        QHash<SessionItem*, SessionTreeItem*> viewItems;
        QHash<SessionModel*, SessionTreeItem*> sessionItems;
        mutable QTreeWidgetItem* dropParent;
        QVariantHash sortOrders;
    } d;
    friend class SessionTreeItem;
};

#endif // SESSIONTREEWIDGET_H
