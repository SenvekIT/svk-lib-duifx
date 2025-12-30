"""
Setup script for DUIFx package.
"""

from setuptools import setup, find_packages
import os
from pathlib import Path

# Read the contents of README file
this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text(encoding='utf-8')

# Read version from __init__.py
version = {}
with open(this_directory / "duifx" / "__init__.py", encoding='utf-8') as f:
    for line in f:
        if line.startswith("__version__"):
            exec(line, version)
            break

setup(
    name="duifx",
    version=version.get("__version__", "0.1.0"),
    author="David Senoner",
    author_email="david.senoner@davtec.it",
    description="A modern PySide6-based GUI framework with integrated USB interface support",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/davidsenoner/DUIFx",
    packages=find_packages(exclude=["tests", "tests.*", "examples", "examples.*"]),
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Desktop Environment",
        "Topic :: Software Development :: User Interfaces",
    ],
    python_requires=">=3.8",
    install_requires=[
        "PySide6>=6.5.0",
        "pyusb>=1.2.0",
    ],
    extras_require={
        "dev": [
            "pytest>=7.0",
            "pytest-qt>=4.0",
            "black>=22.0",
            "flake8>=5.0",
            "mypy>=0.990",
            "build>=0.10",
            "twine>=4.0",
        ],
    },
    include_package_data=True,
    package_data={
        "duifx": [
            "gui/resources/*.qss",
            "gui/resources/image/*",
            "gui/resources/icon/*.svg",
            "gui/ui/*.ui",
        ],
    },
    entry_points={
        "console_scripts": [
            "duifx=duifx.main:main",
        ],
    },
    keywords="gui, PySide6, desktop, ui, framework, usb, can",
    project_urls={
        "Bug Reports": "https://github.com/davidsenoner/DUIFx/issues",
        "Source": "https://github.com/davidsenoner/DUIFx",
        "Documentation": "https://github.com/davidsenoner/DUIFx#readme",
    },
)