@echo off
REM ============================================================
REM  probe_test.bat — ST-Link + hedef MCU canlilik testi
REM  Board ilk takildiginda Claude bunu calistirir: cihaz ID,
REM  flash boyutu, core okunursa baglanti saglam demektir.
REM ============================================================
set PROG="C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"

echo [probe] ST-Link uzerinden SWD baglantisi deneniyor...
%PROG% -c port=SWD mode=UR
echo.
echo [probe] Exit code: %errorlevel%
echo [probe] Yukarida "Device ID", "Device name: STM32U3..." goruyorsan baglanti TAMAM.
