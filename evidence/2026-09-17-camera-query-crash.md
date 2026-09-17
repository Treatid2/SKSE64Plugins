# Separate Camera-tab crash on 0.1.55

The diagnostic owner supplied the archived 12:54:54 BST crash log and matching
diagnostic minidump. Its verified handoff identifies PID 55888, exception thread
26484 and skee64.dll+02ED354, SKSEScaleform_GetRaceSexCameraPos::Call.

Canonical log SHA-256:
99d46e60fe192a0bfb879c643621140eaa2d41b9fc0d0235c998b19ab08a2b40.
Managed dump SHA-256:
1ac47e7318490ca8c87e7debb01546e33820198367ed580af56b6204396b620f.
The owner's retained capture/provenance note is:
L:\Codex\projects\diagnostics\CS-OCU-Rationalisation\diagnostics\crash-log-collection\dump-intake\2026-09-17-125454\handoff.md

The observed failing instruction reads [RDI+0x6C], with noncanonical
RDI=0x53534F2000000010. Bounded disassembly of the retained exact 0.1.55 DLL
(SHA-256 8161A71959F9DDA69B855ABB0739C935D63842347B1ADA28C08F376EBF03CB69)
shows the preceding load at +02ED329: mov rdi, [rbx+0x118].

Source has three unguarded direct accesses to raceMenu->camera.cameraRoot:
GetRaceSexCameraRot, GetRaceSexCameraPos and SetRaceSexCameraPos. That flat-game
camera root sits at 0x50 + 0xA8 + 0x20 = 0x118. The independently established
VR menu layout has no embedded RaceSexCamera: sliderData starts at 0xF8, so
0x118 belongs to slider-array metadata, not a camera pointer. This establishes
a concrete invalid-layout access in the crashing callback, without needing to
attribute memory corruption to another mod. The same mistake affects all three
camera callbacks. Face View uses its separate RoomNode/HmdNode tracking path.

Private inspection of the previously retained CameraEditor script shows that
its input handler invokes GetRaceSexCameraPos to initialise camera movement,
then uses the position and setter for later movement inputs. This explains a
crash after entering Camera and sending input; the user's precise action has
not been independently recorded. No original ActionScript is reproduced here.

The minidump contains a MemoryList stream, not full memory. The requested
RaceSexMenu object bytes at RBX=0x290FF1DC000 were not captured. Therefore this
inspection does NOT claim an object-byte validation or an atomic live snapshot.
Shared cause with the earlier morph-callback crash remains unproven.

0.1.56 fixes shader export resolution only, and was not installed at crash time.
Its installation remains held while the separate Camera-tab handling is resolved.
