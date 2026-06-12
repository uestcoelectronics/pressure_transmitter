@echo off
REM ============================================================
REM  ease-me - Quick Status: mcu_prog
REM  No elevation needed. Prints state file, opens tracker HTML.
REM ============================================================

set WS=C:\PressureTransmitter\docs\MCU programming

echo ================================================================
echo  mcu_prog - CURRENT STATE
echo ================================================================
if exist "%WS%\mcu_prog_state.md" (
    type "%WS%\mcu_prog_state.md"
) else (
    echo ERROR: state file not found: %WS%\mcu_prog_state.md
)

echo.
echo ================================================================
echo  Opening tracker in default browser...
echo ================================================================
if exist "%WS%\mcu_prog_tracker.html" (
    start "" "%WS%\mcu_prog_tracker.html"
) else (
    echo ERROR: tracker not found: %WS%\mcu_prog_tracker.html
)

echo.
echo Next action: see "Next Recommended Task" above.
pause
