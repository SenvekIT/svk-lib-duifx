from pathlib import Path
import sys

def get_resource_path(relative_path):
    """Get absolute path to resource, works for dev and PyInstaller"""
    if getattr(sys, 'frozen', False):
        # PyInstaller bundle
        base_path = Path(sys._MEIPASS) / "duifx" / "gui"
    else:
        # Development
        base_path = Path(__file__).parent
    
    full_path = base_path / relative_path
    return full_path