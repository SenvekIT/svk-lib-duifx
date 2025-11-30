@echo off
REM filepath: build_package.bat
echo ========================================
echo Building DUIFx Package
echo ========================================

REM Cleanup old builds
echo Cleaning old builds...
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist
if exist duifx.egg-info rmdir /s /q duifx.egg-info

REM Remove __pycache__ directories
echo Cleaning __pycache__...
for /d /r . %%d in (__pycache__) do @if exist "%%d" rmdir /s /q "%%d"

REM Remove .pyc files
echo Cleaning .pyc files...
del /s /q *.pyc 2>nul

REM Build the package
echo.
echo Building package...
python -m build

REM Check if build was successful
if %errorlevel% neq 0 (
    echo.
    echo ❌ Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo ✅ Build completed successfully!
echo ========================================
echo.
echo Package files created in dist/:
dir dist
echo.
echo To install locally:
echo   pip install dist\duifx-0.1.0-py3-none-any.whl
echo.
echo To upload to PyPI:
echo   python -m twine upload dist/*
echo.
pause