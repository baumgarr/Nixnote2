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

#include "nmainmenubar.h"
#include "global.h"
#include <QAbstractAnimation>
#include <QFileIconProvider>

extern Global global;

NMainMenuBar::NMainMenuBar(QWidget *parent) :
    QMenuBar(parent)
{
    this->parent = (NixNote*)parent;
    font.setPointSize(8);

    setupFileMenu();
    setupEditMenu();
    setupViewMenu();
    setupToolsMenu();
    setupHelpMenu();
}


void NMainMenuBar::setupFileMenu() {
    fileMenu = this->addMenu(tr("&File"));

    QFont f;
    f.setPointSize(8);

    printAction = new QAction(tr("Print Note"), this);
    printAction->setToolTip(tr("Print this note"));
    printAction->setFont(f);
    connect(printAction, SIGNAL(triggered()), parent, SLOT(printNote()));
    setupShortcut(printAction, QString("File_Print"));
    fileMenu->addAction(printAction);

    restoreDatabaseAction = new QAction(tr("Restore Database"), this);
    restoreDatabaseAction->setToolTip(tr("Restore from a backup"));
    restoreDatabaseAction->setFont(f);
    connect(restoreDatabaseAction, SIGNAL(triggered()), parent, SLOT(databaseRestore()));
    setupShortcut(restoreDatabaseAction, QString("File_Restore_Database"));
    fileMenu->addAction(restoreDatabaseAction);


    fileMenu->addSeparator();
    // Start adding the user accounts
    QList<QString> names = global.accountsManager->nameList();
    QList<int> ids = global.accountsManager->idList();
    for (int i=0; i<ids.size(); i++) {
        QAction *accountAction = new QAction(names[i], this);
        accountAction->setData(ids[i]);
        accountAction->setCheckable(true);
        accountAction->setFont(f);
        if (global.accountsManager->currentId == ids[i])
            accountAction->setChecked(true);
        else {
            accountAction->setText(tr("Switch to ")+names[i]);
        }
        fileMenu->addAction(accountAction);
        connect(accountAction, SIGNAL(triggered()), parent, SLOT(switchUser()));
        userAccountActions.append(accountAction);
    }

    addUserAction = new QAction(tr("Add Another User..."), this);
    addUserAction->setFont(font);
    fileMenu->addAction(addUserAction);
    connect(addUserAction, SIGNAL(triggered()), parent, SLOT(addAnotherUser()));

    userMaintenanceAction = new QAction(tr("User Account Maintenance"),this);
    userMaintenanceAction->setFont(font);
    fileMenu->addAction(userMaintenanceAction);
    connect(userMaintenanceAction, SIGNAL(triggered()), parent, SLOT(userMaintenance()));

    fileMenu->addSeparator();

    exitAction = new QAction(tr("Exit"), this);
    exitAction->setToolTip(tr("Close the program"));
    connect(exitAction, SIGNAL(triggered()), parent, SLOT(closeNixNote()));
    exitAction->setShortcut(QKeySequence::Close);
    exitAction->setIcon(QIcon::fromTheme("exit"));
    exitAction->setFont(f);
    //exitAction->setIcon(QFileIconProvider().icon(QFileInfo("/home/randy/Dropbox/nixnote.jar")));
    setupShortcut(exitAction, QString("File_Exit"));
    fileMenu->addAction(exitAction);
}


void NMainMenuBar::addUserAccount(QAction *action) {
    fileMenu->insertAction(addUserAction, action);
    userAccountActions.append(action);
}


void NMainMenuBar::setupEditMenu() {
    editMenu = this->addMenu(tr("&Edit"));

    undoAction = new QAction(tr("Undo"), this);
    setupShortcut(undoAction, QString("Edit_Undo"));
    undoAction->setFont(font);
    editMenu->addAction(undoAction);

    redoAction = new QAction(tr("Redo"), this);
    setupShortcut(redoAction, QString("Edit_Redo"));
    redoAction->setFont(font);
    editMenu->addAction(redoAction);

    editMenu->addSeparator();

    cutAction = new QAction(tr("Cut"), this);
    setupShortcut(cutAction, QString("Edit_Cut"));
    cutAction->setFont(font);
    editMenu->addAction(cutAction);

    copyAction = new QAction(tr("Copy"), this);
    setupShortcut(copyAction, QString("Edit_Copy"));
    copyAction->setFont(font);
    editMenu->addAction(copyAction);

    pasteAction = new QAction(tr("Paste"), this);
    setupShortcut(pasteAction, QString("Edit_Paste"));
    pasteAction->setFont(font);
    editMenu->addAction(pasteAction);

    pasteAsTextAction = new QAction(tr("Paste as Text"), this);
    setupShortcut(pasteAsTextAction, QString("Edit_Paste_Without_Formatting"));
    pasteAsTextAction->setFont(font);
    editMenu->addAction(pasteAsTextAction);

    editMenu->addSeparator();

    deleteNoteAction = new QAction(tr("Delete"), this);
    setupShortcut(deleteNoteAction, QString("File_Note_Delete"));
    deleteNoteAction->setFont(font);
    //editMenu->addAction(deleteNoteAction);

    selectAllAction = new QAction(tr("Select All"), this);
    setupShortcut(selectAllAction, QString("Edit_Select_All"));
    selectAllAction->setFont(font);
    editMenu->addAction(selectAllAction);

    editMenu->addSeparator();

    findReplaceMenu = editMenu->addMenu(tr("Find and Replace"));
    findReplaceMenu->setFont(font);

    searchNotesAction = new QAction(tr("Search Notes"), this);
    setupShortcut(searchNotesAction, QString("Edit_Search_Notes"));
    searchNotesAction->setFont(font);
    findReplaceMenu->addAction(searchNotesAction);
    connect(searchNotesAction, SIGNAL(triggered()), parent->searchText, SLOT(setFocus()));

    resetSearchAction = new QAction(tr("Reset Search"), this);
    setupShortcut(resetSearchAction, QString("Edit_Reset_Search"));
    resetSearchAction->setFont(font);
    findReplaceMenu->addAction(resetSearchAction);
    connect(resetSearchAction, SIGNAL(triggered()), parent, SLOT(resetView()));

    findReplaceMenu->addSeparator();

    searchFindAction = new QAction(tr("Find in Note"), this);
    setupShortcut(searchFindAction, QString("Edit_Search_Find"));
    searchFindAction->setFont(font);
    findReplaceMenu->addAction(searchFindAction);
    connect(searchFindAction, SIGNAL(triggered()), parent, SLOT(findInNote()));


    searchFindNextAction = new QAction(tr("Find Next"), this);
    setupShortcut(searchFindNextAction, QString("Edit_Search_Find_Next"));
    searchFindNextAction->setFont(font);
    findReplaceMenu->addAction(searchFindNextAction);
    connect(searchFindNextAction, SIGNAL(triggered()), parent, SLOT(findNextInNote()));

    searchFindPrevAction = new QAction(tr("Find Previous"), this);
    setupShortcut(searchFindPrevAction, QString("Edit_Search_Find_Prev"));
    searchFindPrevAction->setFont(font);
    findReplaceMenu->addAction(searchFindPrevAction);
    connect(searchFindPrevAction, SIGNAL(triggered()), parent, SLOT(findPrevInNote()));

    findReplaceMenu->addSeparator();

    searchFindReplaceAction = new QAction(tr("Replace Within Note..."), this);
    setupShortcut(searchFindReplaceAction, QString("Edit_Search_Find_Replace"));
    searchFindReplaceAction->setFont(font);
    findReplaceMenu->addAction(searchFindReplaceAction);
    connect(searchFindReplaceAction, SIGNAL(triggered()), parent, SLOT(findReplaceInNote()));

    editMenu->addSeparator();

    preferencesAction = new QAction(tr("Preferences"), this);
    setupShortcut(preferencesAction, QString("Edit_Preferences"));
    preferencesAction->setFont(font);
    editMenu->addAction(preferencesAction);
    connect(preferencesAction, SIGNAL(triggered()), parent, SLOT(openPreferences()));




}

void NMainMenuBar::setupViewMenu() {
    viewMenu = this->addMenu(tr("&View"));

    viewNoteListWide = new QAction(tr("Wide Note List"), this);
    setupShortcut(viewNoteListWide, "View_Note_List_Wide");
    viewNoteListWide->setFont(font);
    viewMenu->addAction(viewNoteListWide);
    viewNoteListWide->setCheckable(true);

    viewNoteListNarrow = new QAction(tr("Narrow Note List"), this);
    setupShortcut(viewNoteListNarrow, "View_Note_List_Narrow");
    viewNoteListNarrow->setFont(font);
    viewNoteListNarrow->setCheckable(true);
    viewMenu->addAction(viewNoteListNarrow);
    connect(viewNoteListNarrow, SIGNAL(triggered()), parent, SLOT(viewNoteListNarrow()));
    connect(viewNoteListWide, SIGNAL(triggered()), parent, SLOT(viewNoteListWide()));

    viewSourceAction = new QAction(tr("Show Source"), this);
    setupShortcut(viewSourceAction, "View_Source");
    viewSourceAction->setFont(font);
    viewMenu->addAction(viewSourceAction);

    viewMenu->addSeparator();

    viewLeftPanel = new QAction(tr("Show Left Panel"), this);
    setupShortcut(viewLeftPanel, "View_Show_Left_Side");
    viewLeftPanel->setCheckable(true);
    viewLeftPanel->setChecked(true);
    viewLeftPanel->setFont(font);
    viewMenu->addAction(viewLeftPanel);
    connect(viewLeftPanel, SIGNAL(triggered()), parent, SLOT(toggleLeftPanel()));

    viewNoteList = new QAction(tr("Show Note List"), this);
    setupShortcut(viewNoteList, "View_Show_Note_List");
    viewNoteList->setCheckable(true);
    viewNoteList->setChecked(true);
    viewNoteList->setFont(font);
    viewMenu->addAction(viewNoteList);
    connect(viewNoteList, SIGNAL(triggered()), parent, SLOT(toggleNoteList()));

    viewNotePanel = new QAction(tr("Show Note Panel"), this);
    setupShortcut(viewNotePanel, "View_Show_Note_Panel");
    viewNotePanel->setCheckable(true);
    viewNotePanel->setChecked(true);
    viewNotePanel->setFont(font);
    viewMenu->addAction(viewNotePanel);
    connect(viewNotePanel, SIGNAL(triggered()), parent, SLOT(toggleTabWindow()));

    viewMenu->addSeparator();

    viewExtendedInformation = new QAction(tr("View Note Info"), this);
    setupShortcut(viewExtendedInformation, "View_Extended_Information");
    viewExtendedInformation->setFont(font);
    viewMenu->addAction(viewExtendedInformation);

    viewToolbar = new QAction(tr("View Toolbar"), this);
    setupShortcut(viewToolbar, "View_Toolbar");
    viewToolbar->setFont(font);
    viewMenu->addAction(viewToolbar);
    viewToolbar->setCheckable(true);
    viewToolbar->setChecked(true);
    connect(viewToolbar, SIGNAL(triggered()), parent, SLOT(toggleToolbar()));

    viewStatusbar = new QAction(tr("View Statusbar"), this);
    setupShortcut(viewStatusbar, "View_Statusbar");
    viewStatusbar->setFont(font);
    viewMenu->addAction(viewStatusbar);
    viewStatusbar->setCheckable(true);
    //viewStatusbar->setChecked(true);
    connect(viewStatusbar, SIGNAL(triggered()), parent, SLOT(toggleStatusbar()));

}

void NMainMenuBar::setupToolsMenu() {
  toolsMenu = this->addMenu(tr("&Tools"));

  synchronizeAction = new QAction(tr("Synchronize"), this);
  synchronizeAction->setToolTip(tr("Close the program"));
  connect(synchronizeAction, SIGNAL(triggered()), parent, SLOT(synchronize()));
  setupShortcut(synchronizeAction, QString("Tools_Synchronize"));
  synchronizeAction->setFont(font);
  toolsMenu->addAction(synchronizeAction);

  disconnectAction = new QAction(tr("Disconnect"), this);
  synchronizeAction->setToolTip(tr("Disconnect from Evernote"));
  connect(disconnectAction, SIGNAL(triggered()), parent, SLOT(disconnect()));
  setupShortcut(disconnectAction, QString(""));
  disconnectAction->setFont(font);
  toolsMenu->addAction(disconnectAction);
  disconnectAction->setEnabled(false);
  disconnectAction->setVisible(false);  /// We can probably delete this whole menu option

  accountDialogAction = new QAction(tr("Account"), this);
  accountDialogAction->setToolTip(tr("Account information"));
  accountDialogAction->setFont(font);
  connect(accountDialogAction, SIGNAL(triggered()), parent, SLOT(openAccount()));
  setupShortcut(accountDialogAction, QString("Tools_Account_Information"));
  toolsMenu->addAction(accountDialogAction);

  databaseStatusDialogAction = new QAction(tr("Database Status"), this);
  databaseStatusDialogAction->setToolTip(tr("Database Status"));
  setupShortcut(databaseStatusDialogAction, QString("Database_Status"));
  databaseStatusDialogAction->setFont(font);
  connect(databaseStatusDialogAction, SIGNAL(triggered()), parent, SLOT(openDatabaseStatus()));
  toolsMenu->addAction(databaseStatusDialogAction);

  toolsMenu->addSeparator();

  importFoldersDialogAction = new QAction(tr("Import Folders"), this);
  importFoldersDialogAction->setToolTip(tr("Import Folders"));
  importFoldersDialogAction->setFont(font);
  setupShortcut(importFoldersDialogAction, QString("Tools_Import_Folders"));
  connect(importFoldersDialogAction, SIGNAL(triggered()), parent, SLOT(openImportFolders()));
  toolsMenu->addAction(importFoldersDialogAction);


}

void NMainMenuBar::setupHelpMenu() {
    helpMenu = this->addMenu(tr("&Help"));
    aboutAction = new QAction(tr("About"), this);
    aboutAction->setToolTip(tr("About"));
    aboutAction->setFont(font);
    connect(aboutAction, SIGNAL(triggered()), parent, SLOT(openAbout()));
    helpMenu->addAction(aboutAction);
}

void NMainMenuBar::setupShortcut(QAction *action, QString text) {
    if (!global.shortcutKeys->containsAction(&text))
        return;
    QKeySequence key(global.shortcutKeys->getShortcut(&text));
    action->setShortcut(key);
}
