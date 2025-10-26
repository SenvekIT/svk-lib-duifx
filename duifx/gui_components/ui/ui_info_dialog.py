# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'info_dialogySZyBZ.ui'
##
## Created by: Qt User Interface Compiler version 6.10.0
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide6.QtCore import (QCoreApplication, QDate, QDateTime, QLocale,
    QMetaObject, QObject, QPoint, QRect,
    QSize, QTime, QUrl, Qt)
from PySide6.QtGui import (QBrush, QColor, QConicalGradient, QCursor,
    QFont, QFontDatabase, QGradient, QIcon,
    QImage, QKeySequence, QLinearGradient, QPainter,
    QPalette, QPixmap, QRadialGradient, QTransform)
from PySide6.QtWidgets import (QApplication, QDialog, QFrame, QLabel,
    QPlainTextEdit, QSizePolicy, QVBoxLayout, QWidget)

class Ui_InfoDialog(object):
    def setupUi(self, InfoDialog):
        if not InfoDialog.objectName():
            InfoDialog.setObjectName(u"InfoDialog")
        InfoDialog.resize(399, 320)
        self.verticalLayout = QVBoxLayout(InfoDialog)
        self.verticalLayout.setSpacing(0)
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.verticalLayout.setContentsMargins(0, 0, 0, 0)
        self.header = QFrame(InfoDialog)
        self.header.setObjectName(u"header")
        sizePolicy = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.header.sizePolicy().hasHeightForWidth())
        self.header.setSizePolicy(sizePolicy)
        self.header.setMinimumSize(QSize(0, 70))
        self.headerLayout = QVBoxLayout(self.header)
        self.headerLayout.setSpacing(3)
        self.headerLayout.setObjectName(u"headerLayout")
        self.headerLayout.setContentsMargins(10, 10, -1, 10)
        self.labelMainTitle = QLabel(self.header)
        self.labelMainTitle.setObjectName(u"labelMainTitle")

        self.headerLayout.addWidget(self.labelMainTitle)

        self.labelSubTitle = QLabel(self.header)
        self.labelSubTitle.setObjectName(u"labelSubTitle")

        self.headerLayout.addWidget(self.labelSubTitle)


        self.verticalLayout.addWidget(self.header)

        self.content = QFrame(InfoDialog)
        self.content.setObjectName(u"content")
        self.contentLayout = QVBoxLayout(self.content)
        self.contentLayout.setObjectName(u"contentLayout")
        self.textEditInfo = QPlainTextEdit(self.content)
        self.textEditInfo.setObjectName(u"textEditInfo")

        self.contentLayout.addWidget(self.textEditInfo)


        self.verticalLayout.addWidget(self.content)


        self.retranslateUi(InfoDialog)

        QMetaObject.connectSlotsByName(InfoDialog)
    # setupUi

    def retranslateUi(self, InfoDialog):
        InfoDialog.setWindowTitle(QCoreApplication.translate("InfoDialog", u"Info", None))
        self.labelMainTitle.setText(QCoreApplication.translate("InfoDialog", u"labelMainTitle", None))
        self.labelSubTitle.setText(QCoreApplication.translate("InfoDialog", u"labelSubTitle", None))
    # retranslateUi

