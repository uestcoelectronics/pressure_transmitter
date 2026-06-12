@echo off
setlocal enabledelayedexpansion

REM ============================================================
REM  ease-me - Elevated Claude Launcher
REM  Task:    mcu_prog (STM32U385 pressure transmitter firmware)
REM  Repo:    C:\PressureTransmitter
REM  Mode:    start / planning (plan permission mode)
REM
REM  SAFE FLAGS ONLY - no --dangerously-skip-permissions
REM  Self-elevates via UAC if not running as Administrator.
REM ============================================================

REM --- Step 1: Check for admin rights, self-elevate if needed ---
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ease-me] Not running as Administrator. Requesting elevation...
    powershell -NoProfile -Command ^
        "Start-Process cmd.exe -ArgumentList '/c \"%~dpnx0\"' -Verb RunAs -Wait"
    exit /b
)

REM --- Step 2: Locate claude.exe ---
set CLAUDE_EXE=
for /f "tokens=*" %%i in ('where claude 2^>nul') do (
    if not defined CLAUDE_EXE set CLAUDE_EXE=%%i
)
if not defined CLAUDE_EXE (
    set CLAUDE_EXE=%USERPROFILE%\.local\bin\claude.exe
    if not exist "!CLAUDE_EXE!" (
        echo [ease-me] ERROR: claude.exe not found. Install Claude Code or add it to PATH.
        pause
        exit /b 1
    )
    echo [ease-me] Using fallback: !CLAUDE_EXE!
)
echo [ease-me] Claude found: %CLAUDE_EXE%

REM --- Step 3: Navigate to repo root ---
set REPO_PATH=C:\PressureTransmitter
if not exist "%REPO_PATH%" (
    echo [ease-me] ERROR: Repo path not found: %REPO_PATH%
    pause
    exit /b 1
)
cd /d "%REPO_PATH%"
echo [ease-me] Working directory: %CD%

REM --- Step 4: System prompt path ---
set SYSTEM_PROMPT=C:\PressureTransmitter\docs\MCU programming\mcu_prog_claude_system_prompt.md
if not exist "%SYSTEM_PROMPT%" (
    echo [ease-me] WARNING: System prompt not found: %SYSTEM_PROMPT%
    goto :LAUNCH_WITHOUT_PROMPT
)

REM --- Step 5: Launch (planning session: plan permission mode) ---
echo [ease-me] Launching Claude (plan mode) with system prompt:
echo           %SYSTEM_PROMPT%
echo.
"%CLAUDE_EXE%" --permission-mode plan --append-system-prompt-file "%SYSTEM_PROMPT%"
if %errorlevel% equ 0 goto :END

echo [ease-me] Note: launch with flags failed. Falling back...
:LAUNCH_WITHOUT_PROMPT
echo.
echo ================================================================
echo   IMPORTANT: When Claude starts, type:
echo   /ease-me continue
echo   It will read: docs\MCU programming\mcu_prog_memory.md
echo                 docs\MCU programming\mcu_prog_state.md
echo ================================================================
echo.
"%CLAUDE_EXE%"

:END
echo [ease-me] Session ended.
pause
