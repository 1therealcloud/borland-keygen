@echo off
cd /d "%~dp0"
where cl.exe >nul 2>&1 && goto build

for %%E in (Community Professional Enterprise BuildTools) do (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
        goto build
    )
)

echo MSVC 2022 not found.
exit /b 1

:build
if not exist build mkdir build
rc /nologo /I assets /fo build\resource.res assets\resource.rc || exit /b 1
cl /nologo /TC /O2 /MT /D_CRT_SECURE_NO_WARNINGS /Fobuild\ /Febuild\keygen.exe main.c keygen.c /link /MACHINE:X86 /SUBSYSTEM:WINDOWS /MANIFEST:NO build\resource.res user32.lib gdi32.lib shell32.lib kernel32.lib || exit /b 1
