@echo off
setlocal enableextensions

set VCPKG_INSTALL=%VCPKG_INSTALLATION_ROOT%\installed\x64-windows

echo === Compiling SafeGuard.dll ===
cl /nologo /LD /O2 /EHsc /std:c++17 /MD ^
   /I "%VCPKG_INSTALL%\include" ^
   SafeGuard.cpp ^
   "%VCPKG_INSTALL%\lib\imgui.lib" ^
   d3d11.lib dxgi.lib user32.lib gdi32.lib dwmapi.lib psapi.lib ^
   /link /OUT:SafeGuard.dll /MACHINE:X64
if errorlevel 1 exit /b 1

echo === Compiling Launcher.exe ===
cl /nologo /O2 /EHsc Launcher.cpp /link /OUT:Launcher.exe user32.lib /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
if errorlevel 1 exit /b 1

echo === BUILD DONE ===
dir /b *.dll *.exe
