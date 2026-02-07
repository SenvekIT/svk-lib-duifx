# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'main_windowPCcvhU.ui'
##
## Created by: Qt User Interface Compiler version 6.10.0
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide6.QtCore import (QCoreApplication, QDate, QDateTime, QLocale,
    QMetaObject, QObject, QPoint, QRect,
    QSize, QTime, QUrl, Qt)
from PySide6.QtGui import (QAction, QBrush, QColor, QConicalGradient,
    QCursor, QFont, QFontDatabase, QGradient,
    QIcon, QImage, QKeySequence, QLinearGradient,
    QPainter, QPalette, QPixmap, QRadialGradient,
    QTransform)
from PySide6.QtWidgets import (QApplication, QFrame, QHBoxLayout, QLabel,
    QMainWindow, QSizePolicy, QStackedWidget, QTabWidget,
    QVBoxLayout, QWidget)
from . import resource_rc

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        if not MainWindow.objectName():
            MainWindow.setObjectName(u"MainWindow")
        MainWindow.resize(1200, 700)
        sizePolicy = QSizePolicy(QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(MainWindow.sizePolicy().hasHeightForWidth())
        MainWindow.setSizePolicy(sizePolicy)
        MainWindow.setFocusPolicy(Qt.FocusPolicy.TabFocus)
        self.actionExit = QAction(MainWindow)
        self.actionExit.setObjectName(u"actionExit")
        self.actionSettings = QAction(MainWindow)
        self.actionSettings.setObjectName(u"actionSettings")
        self.app = QWidget(MainWindow)
        self.app.setObjectName(u"app")
        font = QFont()
        font.setFamilies([u"Segoe UI"])
        font.setPointSize(10)
        font.setBold(False)
        font.setItalic(False)
        self.app.setFont(font)
        self.appMargins = QVBoxLayout(self.app)
        self.appMargins.setSpacing(0)
        self.appMargins.setObjectName(u"appMargins")
        self.appMargins.setContentsMargins(0, 0, 0, 0)
        self.header = QFrame(self.app)
        self.header.setObjectName(u"header")
        sizePolicy1 = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)
        sizePolicy1.setHeightForWidth(self.header.sizePolicy().hasHeightForWidth())
        self.header.setSizePolicy(sizePolicy1)
        self.header.setMinimumSize(QSize(0, 80))
        self.header.setMaximumSize(QSize(16777215, 80))
        self.horizontalLayout = QHBoxLayout(self.header)
        self.horizontalLayout.setSpacing(0)
        self.horizontalLayout.setObjectName(u"horizontalLayout")
        self.horizontalLayout.setContentsMargins(0, 0, 0, 0)
        self.headerContent = QFrame(self.header)
        self.headerContent.setObjectName(u"headerContent")
        sizePolicy2 = QSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Preferred)
        sizePolicy2.setHorizontalStretch(0)
        sizePolicy2.setVerticalStretch(0)
        sizePolicy2.setHeightForWidth(self.headerContent.sizePolicy().hasHeightForWidth())
        self.headerContent.setSizePolicy(sizePolicy2)
        self.headerContent.setMinimumSize(QSize(0, 0))
        self.headerContentLayout = QHBoxLayout(self.headerContent)
        self.headerContentLayout.setSpacing(10)
        self.headerContentLayout.setObjectName(u"headerContentLayout")
        self.headerContentLayout.setContentsMargins(10, 0, 0, 0)

        self.horizontalLayout.addWidget(self.headerContent)

        self.headerLogo = QFrame(self.header)
        self.headerLogo.setObjectName(u"headerLogo")
        sizePolicy3 = QSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Preferred)
        sizePolicy3.setHorizontalStretch(0)
        sizePolicy3.setVerticalStretch(0)
        sizePolicy3.setHeightForWidth(self.headerLogo.sizePolicy().hasHeightForWidth())
        self.headerLogo.setSizePolicy(sizePolicy3)
        self.headerLogo.setMinimumSize(QSize(150, 0))
        self.headerLogoLayout = QHBoxLayout(self.headerLogo)
        self.headerLogoLayout.setSpacing(0)
        self.headerLogoLayout.setObjectName(u"headerLogoLayout")
        self.headerLogoLayout.setContentsMargins(0, 0, 0, 0)

        self.horizontalLayout.addWidget(self.headerLogo)


        self.appMargins.addWidget(self.header)

        self.content = QFrame(self.app)
        self.content.setObjectName(u"content")
        sizePolicy4 = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Expanding)
        sizePolicy4.setHorizontalStretch(0)
        sizePolicy4.setVerticalStretch(0)
        sizePolicy4.setHeightForWidth(self.content.sizePolicy().hasHeightForWidth())
        self.content.setSizePolicy(sizePolicy4)
        self.content.setFrameShape(QFrame.Shape.NoFrame)
        self.content.setFrameShadow(QFrame.Shadow.Raised)
        self.horizontalLayout_8 = QHBoxLayout(self.content)
        self.horizontalLayout_8.setSpacing(0)
        self.horizontalLayout_8.setObjectName(u"horizontalLayout_8")
        self.horizontalLayout_8.setContentsMargins(0, 0, 0, 0)
        self.sidebar = QFrame(self.content)
        self.sidebar.setObjectName(u"sidebar")
        sizePolicy4.setHeightForWidth(self.sidebar.sizePolicy().hasHeightForWidth())
        self.sidebar.setSizePolicy(sizePolicy4)
        self.sidebar.setMinimumSize(QSize(50, 0))
        self.sidebarLayout = QVBoxLayout(self.sidebar)
        self.sidebarLayout.setSpacing(0)
        self.sidebarLayout.setObjectName(u"sidebarLayout")
        self.sidebarLayout.setContentsMargins(0, 0, 0, 0)
        self.sidebarTop = QFrame(self.sidebar)
        self.sidebarTop.setObjectName(u"sidebarTop")
        sizePolicy5 = QSizePolicy(QSizePolicy.Policy.Maximum, QSizePolicy.Policy.Expanding)
        sizePolicy5.setHorizontalStretch(0)
        sizePolicy5.setVerticalStretch(0)
        sizePolicy5.setHeightForWidth(self.sidebarTop.sizePolicy().hasHeightForWidth())
        self.sidebarTop.setSizePolicy(sizePolicy5)
        self.sidebarTop.setMaximumSize(QSize(50, 16777215))
        self.sidebarTopLayout = QVBoxLayout(self.sidebarTop)
        self.sidebarTopLayout.setObjectName(u"sidebarTopLayout")

        self.sidebarLayout.addWidget(self.sidebarTop)

        self.sidebarBottom = QFrame(self.sidebar)
        self.sidebarBottom.setObjectName(u"sidebarBottom")
        sizePolicy6 = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Minimum)
        sizePolicy6.setHorizontalStretch(0)
        sizePolicy6.setVerticalStretch(0)
        sizePolicy6.setHeightForWidth(self.sidebarBottom.sizePolicy().hasHeightForWidth())
        self.sidebarBottom.setSizePolicy(sizePolicy6)
        self.sidebarBottom.setMinimumSize(QSize(0, 50))
        self.sidebarBottomLayout = QVBoxLayout(self.sidebarBottom)
        self.sidebarBottomLayout.setSpacing(5)
        self.sidebarBottomLayout.setObjectName(u"sidebarBottomLayout")
        self.sidebarBottomLayout.setContentsMargins(5, -1, 5, 5)

        self.sidebarLayout.addWidget(self.sidebarBottom)


        self.horizontalLayout_8.addWidget(self.sidebar)

        self.verticalWidget = QWidget(self.content)
        self.verticalWidget.setObjectName(u"verticalWidget")
        self.main_content = QVBoxLayout(self.verticalWidget)
        self.main_content.setSpacing(0)
        self.main_content.setObjectName(u"main_content")
        self.main_content.setContentsMargins(0, 0, 0, 0)
        self.stackedContent = QFrame(self.verticalWidget)
        self.stackedContent.setObjectName(u"stackedContent")
        self.verticalLayout_4 = QVBoxLayout(self.stackedContent)
        self.verticalLayout_4.setObjectName(u"verticalLayout_4")
        self.stackedWidgetContent = QStackedWidget(self.stackedContent)
        self.stackedWidgetContent.setObjectName(u"stackedWidgetContent")

        self.verticalLayout_4.addWidget(self.stackedWidgetContent)


        self.main_content.addWidget(self.stackedContent)

        self.bottomPanel = QFrame(self.verticalWidget)
        self.bottomPanel.setObjectName(u"bottomPanel")
        sizePolicy7 = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred)
        sizePolicy7.setHorizontalStretch(0)
        sizePolicy7.setVerticalStretch(0)
        sizePolicy7.setHeightForWidth(self.bottomPanel.sizePolicy().hasHeightForWidth())
        self.bottomPanel.setSizePolicy(sizePolicy7)
        self.bottomPanel.setMinimumSize(QSize(0, 200))
        self.bottomPanel.setMaximumSize(QSize(16777215, 200))
        self.bottomPanel.setFrameShape(QFrame.Shape.NoFrame)
        self.bottomPanel.setFrameShadow(QFrame.Shadow.Raised)
        self.verticalLayout_2 = QVBoxLayout(self.bottomPanel)
        self.verticalLayout_2.setSpacing(0)
        self.verticalLayout_2.setObjectName(u"verticalLayout_2")
        self.verticalLayout_2.setContentsMargins(0, 0, 0, 0)
        self.tabWidgetBottomPanel = QTabWidget(self.bottomPanel)
        self.tabWidgetBottomPanel.setObjectName(u"tabWidgetBottomPanel")
        sizePolicy8 = QSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        sizePolicy8.setHorizontalStretch(0)
        sizePolicy8.setVerticalStretch(0)
        sizePolicy8.setHeightForWidth(self.tabWidgetBottomPanel.sizePolicy().hasHeightForWidth())
        self.tabWidgetBottomPanel.setSizePolicy(sizePolicy8)
        self.tabWidgetBottomPanel.setTabsClosable(True)

        self.verticalLayout_2.addWidget(self.tabWidgetBottomPanel)


        self.main_content.addWidget(self.bottomPanel)


        self.horizontalLayout_8.addWidget(self.verticalWidget)


        self.appMargins.addWidget(self.content)

        self.footer = QFrame(self.app)
        self.footer.setObjectName(u"footer")
        self.footer.setMinimumSize(QSize(0, 22))
        self.footer.setMaximumSize(QSize(16777215, 22))
        self.horizontalLayout_5 = QHBoxLayout(self.footer)
        self.horizontalLayout_5.setSpacing(0)
        self.horizontalLayout_5.setObjectName(u"horizontalLayout_5")
        self.horizontalLayout_5.setContentsMargins(0, 0, 0, 0)
        self.lbl_bottomLine = QLabel(self.footer)
        self.lbl_bottomLine.setObjectName(u"lbl_bottomLine")
        self.lbl_bottomLine.setMaximumSize(QSize(16777215, 16))
        self.lbl_bottomLine.setFont(font)
        self.lbl_bottomLine.setCursor(QCursor(Qt.CursorShape.ArrowCursor))
        self.lbl_bottomLine.setAlignment(Qt.AlignmentFlag.AlignRight|Qt.AlignmentFlag.AlignTrailing|Qt.AlignmentFlag.AlignVCenter)

        self.horizontalLayout_5.addWidget(self.lbl_bottomLine)


        self.appMargins.addWidget(self.footer)

        MainWindow.setCentralWidget(self.app)

        self.retranslateUi(MainWindow)

        QMetaObject.connectSlotsByName(MainWindow)
    # setupUi

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(QCoreApplication.translate("MainWindow", u"Application Title", None))
        self.actionExit.setText(QCoreApplication.translate("MainWindow", u"Exit", None))
        self.actionSettings.setText(QCoreApplication.translate("MainWindow", u"Settings", None))
        self.lbl_bottomLine.setText("")
    # retranslateUi

