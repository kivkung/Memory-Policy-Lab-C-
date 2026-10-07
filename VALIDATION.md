# Verification — 7 October 2026

Built with MSYS2 UCRT64 GCC 16.2.0 for Windows x64. Build completed with `-Wall -Wextra -Wpedantic` and no warnings.

- Existing simulator and parser tests passed, including known outputs, step invariants, exhaustive Optimal oracle, UTF-8 BOM, invalid input and limits.
- Existing search tests passed: enumeration oracle, both winners, invalid configurations, budget boundaries and save/reload.
- Existing results directory and path tests passed.
- Existing CLI integration tests passed after retaining the CLI as a separate build target.
- Existing results CLI integration tests passed: file listing, automatic names, alternate working directory, full paths, invalid and empty files.
- Native GUI workflow smoke tests passed: controls enabled/disabled appropriately, fresh runs, next/remaining/stop/restart, three-algorithm summary and paired traces.
- FIFO winner example: FIFO 4 faults / LRU 5 faults, 3 frames.
- Belady example: FIFO 9 faults at 3 frames / 10 faults at 4 frames.
- Worker search returns first FIFO winner at candidate 302; exhausted and limit states preserve prior references/frames. Non-integer input is rejected.
- Save/reload through a Thai filename passed. Invalid file input preserves previous references.
- Maximum 1,000 references, 10 frames and large page IDs passed; full reference viewer preserves all values.
- Search controls fit the minimum supported window dimensions. Four native window render captures were inspected; selected combo values are included by the control's print handler.
- PE import inspection found only Windows system DLLs and Windows Universal CRT API sets, with no MSYS2/GCC runtime DLL dependency.

The GUI smoke suite invokes the application's native command handlers and checks control contents/state. It tests file read/write behind the Windows picker, not manual clicks through the operating system's file dialog. Render captures are offscreen native-control exports, not a recording of manual desktop use. The UI's minimum window is approximately 1100 × 850 at 100% display scale.

Reproduce with `test.bat`; screenshots and the GUI log are generated under `build/gui-qa`.
