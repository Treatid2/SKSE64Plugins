# Runtime SWF adapter: SKSE opener regression

0.1.54-r5 was tested in the human profile. New-game character creation was
skipped. The retained RaceMenuNGVR2 log reports Prepare refusing the opener
identity at 12:29:37.507 BST, before reconstruction or a served movie. This is
a failed load, not an ordinary accepted character-creation exit. The native
failed-load path allows gameplay to proceed; it must not be described as a
successful end-to-end fail-closed character-creation workflow.

Read-only inspection at 2026-09-17T11:40:10Z observed live SkyrimVR PID 10412,
started 11:27:18Z, exact Steam executable. The helper opens only
QUERY_LIMITED_INFORMATION | VM_READ, never attaches a debugger, suspends
threads, invokes code, or writes process memory. Its reads follow the qualified
manager/loader/state-bag pointer chain and at most eight links in the sole
type-10 hash bucket. It is a non-atomic live observation; native table and
opener-object identity are checked for changes across the observation.

Native BSScaleformFileOpener table RVA 0x1866248 retains slots F22070, F20AC0,
F20B30, F20B40; opener and memory-file constructor code prefixes match the
previous immutable-dump qualification. The active type-10 object instead has
a vtable at sksevr_1_4_15.dll + F6140 with slots 17F40, 12630, 6AC0, 17D40.
Offline PE RTTI identifies SKSEFileLoader. Bounded disassembly shows slot 1
delegating to vanilla OpenFile and slot 3 retaining SKSE's path fallback while
calling vanilla OpenFileEx directly. Therefore hooking the vanilla table alone
would also miss the active SKSE wrapper route.

SKSE DLL disk SHA-256:
633BE638897343625D755A6207C9C805AA1B9E82245FBEF1B3B154C91A973463.
Actual loaded OpenFileEx first-192-byte SHA-256 matches independent PE bytes:
FFEB7A9C92A0980453766C4FB79D94B76F3FB88CD25C412DBEF0060D09989749.

Correction in 0.1.55/native 0.5.0.65 accepts precisely this qualified wrapper
or the qualified vanilla opener. It hooks the active table, chaining its
original slot 3 for every unrelated URL. All four SKSE table entries and loaded
OpenFileEx code hash must match; unknown wrappers and modified functions remain
rejected. Original, patch, reconstructed movie and native engine signatures
retain their existing strict checks. Identity failures now log individual
observed/expected values. No menu layout, asset or input logic is changed.

Retained live receipt:
L:\Codex\artifacts\RaceMenu-VR-2\runtime-swf-patch\live-opener-0154-failure.json.
Failure log retained by the existing profile owner under its 0.1.54 deployment
evidence directory. The helper and the source correction are reviewable in Git.
Successful compilation/offline reconstruction is not runtime qualification;
the corrected adapter still needs the live character-creation test.

The corrected native build passed its private exact-reconstruction and malformed
patch tests, and the reference tests passed. The unchanged patch payload is
packaged without the original SWF or other upstream assets.

Candidate archive: RaceMenu-VR2-0.1.55-runtime-swf-opener-TEST.zip, retained under
the runtime-swf-patch artifact directory. SHA-256:
1A39F6AF237BC4E78547ED4BDCBDC3A3750EC8EFC33E0E0855077136C88E541D.
All 77 ZIP members were streamed and independently checked against the exact
staging manifest for member path, byte length and SHA-256. Native DLL SHA-256:
8161A71959F9DDA69B855ABB0739C935D63842347B1ADA28C08F376EBF03CB69.
The manifest is addon-stage-0155-receipt.json; native build output is retained
in native-build-0155.log. These are candidate build checks, not live acceptance.
