/********************************************************************************
** Form generated from reading UI file 'alpr_Interface.ui'
**
** Created by: Qt User Interface Compiler version 5.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ALPR_INTERFACE_H
#define UI_ALPR_INTERFACE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_alpr_InterfaceClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *alpr_InterfaceClass)
    {
        if (alpr_InterfaceClass->objectName().isEmpty())
            alpr_InterfaceClass->setObjectName(QStringLiteral("alpr_InterfaceClass"));
        alpr_InterfaceClass->resize(600, 400);
        menuBar = new QMenuBar(alpr_InterfaceClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        alpr_InterfaceClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(alpr_InterfaceClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        alpr_InterfaceClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(alpr_InterfaceClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        alpr_InterfaceClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(alpr_InterfaceClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        alpr_InterfaceClass->setStatusBar(statusBar);

        retranslateUi(alpr_InterfaceClass);

        QMetaObject::connectSlotsByName(alpr_InterfaceClass);
    } // setupUi

    void retranslateUi(QMainWindow *alpr_InterfaceClass)
    {
        alpr_InterfaceClass->setWindowTitle(QApplication::translate("alpr_InterfaceClass", "alpr_Interface", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class alpr_InterfaceClass: public Ui_alpr_InterfaceClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ALPR_INTERFACE_H
