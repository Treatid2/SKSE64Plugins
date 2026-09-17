# Runtime shader compiler export correction

RaceMenuNGVR2.log from native 0.5.0.65 reports repeated failures resolving
REX::W32::D3DCompile. The production helper in skee64/CDXShaderCompile.cpp
passed that namespace-qualified spelling to GetProcAddress. DLL export names
are ABI strings; the real compiler export is D3DCompile. Microsoft documents
the API and d3dcompiler_47.dll at:
https://learn.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile

The correction changes the lookup and error message, without changing compiler
flags, shader source, version fallback selection or precompiled shader fallback.
A bounded search of skee64 C++ headers/sources found no other namespace-qualified
GetProcAddress export strings.

The new native test compiles the production CDXShaderCompile.cpp and uses its
real DLL lookup and compiler. It compiles all seven CharGen sculpt/brush shaders,
the NiOverride texture vertex shader and all 15 NiOverride tint effect pixel
shaders. It also verifies invalid-argument rejection and genuine compiler
diagnostics for malformed HLSL. Release checks throw instead of relying on
assertions disabled by NDEBUG. No game process, GPU or fallback shader bytecode
is used in this test. It runs as a required post-build dependency of the VR DLL.

0.1.56-shader-compiler / native 0.5.0.66 compiled successfully. The production
test reported all 23 sources compiled and both error-handling checks passed.
Native SWF patch policy tests also passed. Runtime patch payload, menus and
upstream assets are unchanged. Retained build log:
L:\Codex\artifacts\RaceMenu-VR-2\runtime-swf-patch\native-build-0156.log

Candidate DLL SHA-256:
1D637DD29A6AFAC28B2B04CAD50F85C2B65C528E74AFF56525DCDA7C477A18AE.
Native artifacts/manifest are retained in runtime-swf-patch/native-0156.

The user then reported a CTD on the still-installed 0.1.55 and will supply crash
evidence. Installation of 0.1.56 is held pending that evidence classification.
This correction is offline-tested, not yet live-qualified or publicly released.
