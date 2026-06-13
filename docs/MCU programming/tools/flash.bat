@echo off
REM ============================================================
REM  flash.bat — PressureTransmitter firmware'i ST-Link/SWD ile yükle
REM  Claude bunu doğrudan Bash ile de çağırabilir; elle de çalışır.
REM  Gereksinim: ST-Link board'a + USB'ye takılı olmalı.
REM ============================================================
set PROG="C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
set ELF=C:\PressureTransmitter\Firmware\build\Debug\PressureTransmitter.elf

if not exist "%ELF%" (
    echo [flash] ELF yok: %ELF%
    echo [flash] Once derle:  cmake --build build\Debug
    exit /b 1
)

echo [flash] Yukleniyor: %ELF%
REM mode=UR (connect-under-reset) = SWD pinleri yeniden konfigure edilse bile guvenli
%PROG% -c port=SWD mode=UR reset=HWrst -d "%ELF%" --verify -rst
echo [flash] Exit code: %errorlevel%
