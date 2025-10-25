import json
import sys
import logging

from PySide6.QtWidgets import QWidget
from PySide6.QtCore import Signal

from app.gui_components.windows import Container
from app.gui_components.ui.ui_testmanager_widget import Ui_TestManager

logger = logging.getLogger(__name__)
           
class TestManagerWidget(QWidget):
    def __init__(self, container: Container = None):
        super().__init__()

        # get UI
        self.ui = Ui_TestManager()
        self.ui.setupUi(self)

        if container and not isinstance(container, Container):
            raise ValueError("container not a Container instance")
        
        container.set_content(self)

    def set_main_title(self, title: str) -> None:
        """Set the main title of the test manager widget"""
        self.ui.labelMainTitle.setText(title)

    def set_sub_title(self, subtitle: str) -> None:
        """Set the subtitle of the test manager widget"""
        self.ui.labelSubTitle.setText(subtitle)
        