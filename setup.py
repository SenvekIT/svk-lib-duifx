"""
Setup script for DUIFx package.
"""

from setuptools import setup, find_packages
import os

# Read the contents of README file
this_directory = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(this_directory, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name="duifx",
    version="0.1.0",
    author="David Senoner",
    author_email="david.senoner@davtec.it",
    description="A modern PySide6-based GUI framework",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/davidsenoner/DUIFx",
    packages=find_packages(),
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
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Desktop Environment",
        "Topic :: Software Development :: User Interfaces",
    ],
    python_requires=">=3.8",
    install_requires=[
        "PySide6>=6.0.0"
    ],
    extras_require={
        "dev": [
            "pytest>=6.0",
            "pytest-qt>=4.0",
            "black>=21.0",
            "flake8>=3.8",
            "mypy>=0.800",
        ],
    },
    include_package_data=True,
    package_data={
        "duifx": [
            "gui/resources/*.qss",
            "gui/resources/image/*",
            "gui/resources/icon/*",
            "gui/ui/*.ui",
        ],
    },
    entry_points={
        "console_scripts": [
            "duifx=duifx.main:main",
        ],
    },
    keywords="gui, PySide6, desktop, ui, framework",
    project_urls={
        "Bug Reports": "https://github.com/davidsenoner/DUIFx/issues",
        "Source": "https://github.com/davidsenoner/DUIFx",
        "Documentation": "https://github.com/davidsenoner/DUIFx#readme",
    },
)