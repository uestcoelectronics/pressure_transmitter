# mcu_prog — Project System Prompt (ease-me)

You are working on the **mcu_prog** task: STM32U385RGT7 pressure transmitter firmware (4-20 mA config) in repo `C:\PressureTransmitter`. Communicate with the user in **Turkish** (technical terms may stay English).

## Startup (MANDATORY, before any work)
Read these files in `C:\PressureTransmitter\docs\MCU programming\`:
1. `mcu_prog_memory.md` (compact context)
2. `mcu_prog_state.md` (authoritative state)
3. `mcu_prog_roadmap.md` + `mcu_prog_backlog.md` (when selecting/executing work)
4. `mcu_prog_changelog.md` (last entries)
5. `mcu_prog_project_profile.md`
Do NOT rely on chat history alone — these files are the source of truth.

## Process rules
- Separate planning from implementation. Do not modify production code unless the user explicitly approved executing a specific card (`/ease-me execute CARD-ID`).
- Before editing production files, pass the ease-me Pre-Implementation Gate (see skill). If any item fails, stop and ask the single blocking question.
- Respect each card's "Files allowed to edit" list and diff budget (default: max 3 changed + 3 new production files). No unrelated refactors, no architecture rewrites.
- FORBIDDEN: `Firmware/Drivers/**`, `Firmware/cmake/**`, `startup_*.s`, `*.ld`, `Core/**` outside USER CODE blocks, hand-editing `PressureTransmitter.ioc` (CubeMX changes are manual steps for the user).
- The repo is NOT a git repository until CARD-0.3 completes. Before any production edit, create a `<file>.bak` backup copy (or verify git exists).
- Pin truth source: `PIN_MAPPING.xlsx` (MCU Pin Map sheet). BOM truth: `BOM/PCBPT910G1_4_20.xlsx`.
- Hardware, CubeMX, schematic-reading, and BLE phone tests are MANUAL steps — log them in `mcu_prog_manual_steps.md`, never claim them done.
- Run the validation ladder honestly: report the highest level actually performed (build = level 2). Never claim hardware validation without user-provided results.
- Build command: `cd C:\PressureTransmitter\Firmware && cmake --preset Debug && cmake --build build/Debug`.

## After every task
Update: `mcu_prog_state.md`, `mcu_prog_changelog.md` (append), `mcu_prog_tracker.html` (data object), and `mcu_prog_memory.md` if architecture facts changed. Append a task packet to `mcu_prog_task_packets.md` for executed cards. End your report with validation level reached, what was NOT validated, and a "NEXT PROMPT TO CLAUDE" block.

## Parallelism
Use subagents for independent read-only research (datasheets, audits). Never parallelize conflicting code edits.
