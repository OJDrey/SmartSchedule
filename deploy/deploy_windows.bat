@echo off
setlocal

REM ===== EDIT THESE PATHS FOR YOUR Qt INSTALLATION (forward or back slashes both work) =====
REM QT_KIT   = Qt version folder that contains bin/qmake.exe, e.g. C:/Qt/6.8.0/mingw_64
REM MINGW_BIN = folder with g++.exe,                      e.g. C:/Qt/Tools/mingw1310_64/bin
REM TOOLS_DIR = folder with CMake_64 and Ninja,           e.g. C:/Qt/Tools
REM NSIS_DIR  = folder with makensis.exe (needed only for the .exe installer)
set QT_KIT=C:\ProgramData\QtOnline\6.12.0\mingw_64
set MINGW_BIN=C:\ProgramData\QtOnline\Tools\mingw1310_64\bin
set TOOLS_DIR=C:\ProgramData\QtOnline\Tools
set NSIS_DIR=C:\Program Files (x86)\NSIS
REM ========================================================================================

set "QT_KIT=%QT_KIT:\=/%"
set "MINGW_BIN=%MINGW_BIN:\=/%"
set "TOOLS_DIR=%TOOLS_DIR:\=/%"
set "NSIS_DIR=%NSIS_DIR:\=/%"

if not exist "%QT_KIT%/bin/qmake.exe" (
    echo [ERROR] Qt not found in "%QT_KIT%"
    echo Fix QT_KIT: it must be the Qt version folder that contains bin/qmake.exe
    goto error
)
if not exist "%MINGW_BIN%/g++.exe" (
    echo [ERROR] g++.exe not found in "%MINGW_BIN%"
    echo Fix MINGW_BIN: look for Tools/mingw..._64/bin inside your Qt folder
    goto error
)
if not exist "%TOOLS_DIR%/Ninja/ninja.exe" (
    echo [ERROR] ninja.exe not found in "%TOOLS_DIR%/Ninja"
    echo Fix TOOLS_DIR
    goto error
)
if not exist "%TOOLS_DIR%/CMake_64/bin/cmake.exe" (
    echo [ERROR] cmake.exe not found in "%TOOLS_DIR%/CMake_64/bin"
    echo Fix TOOLS_DIR
    goto error
)

set "PATH=%QT_KIT%/bin;%MINGW_BIN%;%TOOLS_DIR%/CMake_64/bin;%TOOLS_DIR%/Ninja;%NSIS_DIR%;%PATH%"

cd /d "%~dp0.."

if exist build-release rmdir /s /q build-release

echo [1/4] Configuring project (Release)...
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_KIT% -DCMAKE_CXX_COMPILER=%MINGW_BIN%/g++.exe
if errorlevel 1 goto error

echo [2/4] Building...
cmake --build build-release --parallel
if errorlevel 1 goto error

echo [3/4] Deploying Qt libraries (windeployqt) into dist folder...
cmake --install build-release --prefix dist
if errorlevel 1 goto error

echo [4/4] Creating ZIP archive and installer (CPack)...
cd build-release
cpack -G ZIP
if errorlevel 1 goto error

where makensis >nul 2>nul
if errorlevel 1 goto nonsis
cpack -G NSIS
if errorlevel 1 goto error
goto packed

:nonsis
echo.
echo [WARNING] makensis.exe not found, the .exe installer was skipped.
echo Install NSIS from nsis.sourceforge.io and fix NSIS_DIR at the top of this script.

:packed

cd ..
echo.
echo DONE. The ZIP archive and the installer if NSIS was found are in the build-release folder.
echo Portable version without installer: dist/bin
goto end

:error
echo.
echo Build stopped. Read the message above.
pause
exit /b 1

:end
pause
endlocal
