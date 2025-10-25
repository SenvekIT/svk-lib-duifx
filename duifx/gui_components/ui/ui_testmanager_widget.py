# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'testmanager_widgetGOBngo.ui'
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
from PySide6.QtWidgets import (QApplication, QCheckBox, QFrame, QLabel,
    QPushButton, QRadioButton, QSizePolicy, QVBoxLayout,
    QWidget)

class Ui_TestManager(object):
    def setupUi(self, TestManager):
        if not TestManager.objectName():
            TestManager.setObjectName(u"TestManager")
        TestManager.resize(398, 297)
        self.verticalLayout = QVBoxLayout(TestManager)
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.verticalLayout.setContentsMargins(5, 5, 5, 5)
        self.labelMainTitle = QLabel(TestManager)
        self.labelMainTitle.setObjectName(u"labelMainTitle")
        sizePolicy = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.labelMainTitle.sizePolicy().hasHeightForWidth())
        self.labelMainTitle.setSizePolicy(sizePolicy)

        self.verticalLayout.addWidget(self.labelMainTitle)

        self.labelSubTitle = QLabel(TestManager)
        self.labelSubTitle.setObjectName(u"labelSubTitle")

        self.verticalLayout.addWidget(self.labelSubTitle)

        self.verticalFrame = QFrame(TestManager)
        self.verticalFrame.setObjectName(u"verticalFrame")
        sizePolicy1 = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Expanding)
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)
        sizePolicy1.setHeightForWidth(self.verticalFrame.sizePolicy().hasHeightForWidth())
        self.verticalFrame.setSizePolicy(sizePolicy1)
        self.verticalLayout_2 = QVBoxLayout(self.verticalFrame)
        self.verticalLayout_2.setObjectName(u"verticalLayout_2")
        self.pushButton = QPushButton(self.verticalFrame)
        self.pushButton.setObjectName(u"pushButton")

        self.verticalLayout_2.addWidget(self.pushButton)

        self.radioButton = QRadioButton(self.verticalFrame)
        self.radioButton.setObjectName(u"radioButton")

        self.verticalLayout_2.addWidget(self.radioButton)

        self.checkBox = QCheckBox(self.verticalFrame)
        self.checkBox.setObjectName(u"checkBox")

        self.verticalLayout_2.addWidget(self.checkBox)


        self.verticalLayout.addWidget(self.verticalFrame)


        self.retranslateUi(TestManager)

        QMetaObject.connectSlotsByName(TestManager)
    # setupUi

    def retranslateUi(self, TestManager):
        TestManager.setWindowTitle(QCoreApplication.translate("TestManager", u"TestManager Content", None))
        self.labelMainTitle.setText(QCoreApplication.translate("TestManager", u"labelMainTitle", None))
        self.labelSubTitle.setText(QCoreApplication.translate("TestManager", u"labelSubTitle", None))
        self.pushButton.setText(QCoreApplication.translate("TestManager", u"PushButton", None))
        self.radioButton.setText(QCoreApplication.translate("TestManager", u"RadioButton", None))
        self.checkBox.setText(QCoreApplication.translate("TestManager", u"CheckBox", None))
    # retranslateUi

