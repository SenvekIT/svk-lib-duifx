# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'settings_dialogxAHOGn.ui'
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
from PySide6.QtWidgets import (QAbstractButton, QApplication, QDialog, QDialogButtonBox,
    QFrame, QLabel, QSizePolicy, QVBoxLayout,
    QWidget)

class Ui_SettingsDialog(object):
    def setupUi(self, SettingsDialog):
        if not SettingsDialog.objectName():
            SettingsDialog.setObjectName(u"SettingsDialog")
        SettingsDialog.resize(923, 692)
        self.verticalLayout = QVBoxLayout(SettingsDialog)
        self.verticalLayout.setSpacing(0)
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.verticalLayout.setContentsMargins(0, 0, 0, 0)
        self.header = QFrame(SettingsDialog)
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

        self.content = QFrame(SettingsDialog)
        self.content.setObjectName(u"content")
        self.contentLayout = QVBoxLayout(self.content)
        self.contentLayout.setObjectName(u"contentLayout")

        self.verticalLayout.addWidget(self.content)

        self.verticalFrame = QFrame(SettingsDialog)
        self.verticalFrame.setObjectName(u"verticalFrame")
        sizePolicy.setHeightForWidth(self.verticalFrame.sizePolicy().hasHeightForWidth())
        self.verticalFrame.setSizePolicy(sizePolicy)
        self.footer = QVBoxLayout(self.verticalFrame)
        self.footer.setSpacing(0)
        self.footer.setObjectName(u"footer")
        self.footer.setContentsMargins(10, 5, 10, 10)
        self.buttonBox = QDialogButtonBox(self.verticalFrame)
        self.buttonBox.setObjectName(u"buttonBox")
        self.buttonBox.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.buttonBox.setOrientation(Qt.Orientation.Horizontal)
        self.buttonBox.setStandardButtons(QDialogButtonBox.StandardButton.Cancel|QDialogButtonBox.StandardButton.Ok)

        self.footer.addWidget(self.buttonBox)


        self.verticalLayout.addWidget(self.verticalFrame)


        self.retranslateUi(SettingsDialog)

        QMetaObject.connectSlotsByName(SettingsDialog)
    # setupUi

    def retranslateUi(self, SettingsDialog):
        SettingsDialog.setWindowTitle(QCoreApplication.translate("SettingsDialog", u"Settings", None))
        self.labelMainTitle.setText(QCoreApplication.translate("SettingsDialog", u"labelMainTitle", None))
        self.labelSubTitle.setText(QCoreApplication.translate("SettingsDialog", u"labelSubTitle", None))
    # retranslateUi

