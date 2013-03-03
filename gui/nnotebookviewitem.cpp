/*********************************************************************************
NixNote - An open-source client for the Evernote service.
Copyright (C) 2013 Randy Baumgarte

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
***********************************************************************************/

#include "nnotebookviewitem.h"
#include "sql/sharednotebooktable.h"
#include "sql/linkednotebooktable.h"
#include "sql/notebooktable.h"


/*
 * These constructors are somewhat confusing
 * If they are like they should be, a comment about the choices here
 * would be really helpful
 */

NNotebookViewItem::NNotebookViewItem(qint32 lid, QTreeWidget* parent):QTreeWidgetItem(parent) {
    count = 0;
    this->setType(lid);
    this->lid = lid;
}


NNotebookViewItem::NNotebookViewItem(qint32 lid):QTreeWidgetItem(){
    count = 0;
    this->type = type; // what?
    this->lid = lid;
    this->setType(lid);
}

void NNotebookViewItem::setRootColor(bool val) {
    if (val) {
        QFont f;
        f.setBold(true);
        setFont(0, f);
    }
}


bool NNotebookViewItem::operator<(const QTreeWidgetItem &other)const {
    int column = treeWidget()->sortColumn();
    return text(column).toLower() < other.text(column).toLower();
}



void NNotebookViewItem::setType(NNotebookWidgetType type) {
    this->type = type;
    if (type == Linked)
        setIcon(0, QIcon(":notebook-linked.png"));
    if (type == Shared)
        setIcon(0, QIcon(":notebook-shared.png"));
    if (type == Local)
        setIcon(0, QIcon(":notebook-local.png"));
    if (type == Synchronized)
        setIcon(0, QIcon(":notebook_small.png"));
    if (type == Stack)
        setIcon(0, QIcon(":stack.png"));
    if (type == Conflict)
        setIcon(0, QIcon(":notebook-conflict.png"));
}


void NNotebookViewItem::setType(qint32 type) { // Problem: type is not used, and lid may not initialized (see constructor)
    LinkedNotebookTable linkedTable;
    SharedNotebookTable sharedTable;
    NotebookTable bookTable;

    if (lid == 0) {
        this->setType(Stack);
        return;
    }
    if (linkedTable.exists(lid)) {
        this->setType(Linked);
        return;
    }

    if (sharedTable.exists(lid)) {
        this->setType(Shared);
        return;
    }
    if (!bookTable.isLocal(lid)) {
        this->setType(Synchronized);
        return;
    }

    Notebook notebook;
    bookTable.get(notebook, lid);
    if (QString::fromStdString(notebook.name).startsWith("Conflict", Qt::CaseInsensitive)) {
        this->setType(Conflict);
        return;
    }
    this->setType(Local);
    return;

}



