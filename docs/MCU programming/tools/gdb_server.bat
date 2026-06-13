@echo off
REM ============================================================
REM  gdb_server.bat — ST-LINK GDB server'i baslat (arka plan)
REM  Claude bunu background'da calistirir; sonra arm-none-eabi-gdb
REM  localhost:61234'e baglanip kaynak-seviyesi debug yapar.
REM  Gereksinim: ST-Link board'a + USB'ye takili.
REM ============================================================
set GDBSRV="C:\Users\Alper\AppData\Local\stm32cube\bundles\stlink-gdbserver\7.13.0+st.3\bin\ST-LINK_gdbserver.exe"
set CUBEBIN="C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"

echo [gdbserver] Baslatiliyor (SWD, port 61234)...
REM  -p port  -cp CubeProgrammer-bin  -d SWD  -e shared  -r 1 hard-reset  -k stay-alive
%GDBSRV% -p 61234 -cp %CUBEBIN% -d -e -r 1 -k
