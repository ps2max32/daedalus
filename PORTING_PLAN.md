# DaedalusX64 on the PS2 (maxsdk): porting plan

Repository: ps2max32/daedalus (a fork; `upstream` DaedalusX64/daedalus, based on `580c0c40` of 2026-10-10, GPL-2), branch `ps2`. A port in the style of njemu:
upstream `Source/` untouched where possible, a new platform `Source/SysPS2/` and a
`DAEDALUS_PS2` define, built with a Makefile on maxsdk (not upstream's CMake).

## What the survey found

**Size.** Core 11k lines, dynarec common 6k + MIPS backend 7.5k, HLE graphics 14k, HLE
audio 7k, PSP platform 14.5k (of which we reuse the renderer half).

**CPU.**
- A pure C++ R4300 interpreter exists (`Core/Interpret.cpp`), and the dynarec can be off
  (`DynaRec/dummy`). The RSP is HLE only (gfx, audio, JPEG, HVQM; `Core/RSP_HLE.cpp:253`).
- The MIPS dynarec (`DynaRec/mips`, written for the PSP's Allegrex) emits only MIPS II
  instructions plus `EXT`/`INS` (`AssemblyWriterPSP.h`). N64 64-bit registers are kept as
  hi/lo pairs of 32-bit registers (`N64RegisterCache.h`). EE gaps:
  - `EXT`/`INS`: 5 sites in `CodeGeneratorPSP.cpp` (4339, 4382, 4651, 4959: the FPU
    condition bit) and the float/double stubs in `DynaRecStubs.S:446-554` (also `clz`; the EE
    has `movz`, not `clz`).
  - FPU: the emitter uses `TRUNC.W.S` and `FLOOR.W.S`, which the EE lacks; the EE's
    `CVT.W.S` always truncates (EE Core Instruction Set Manual), so N64 code converting
    in round-to-nearest mode needs a sequence.
  - The icache flush uses `cache 8` (Allegrex); the EE's hit invalidate is `IHIN` (op 0x0B),
    and code written through a cached pointer needs a D-cache writeback first. CACHE works in
    our user mode (CU0 is set: maxsdk `docs/bios.md`).
  - `$k0` was used as a load base (`CodeGeneratorPSP.cpp:266`, commented out now): the
    BIOS's interrupt path clobbers k0/k1, so they must stay out of the allocator.
  - The R5900 short-loop erratum applies to generated code too: no compiler pads it for us.
- Doubles are run as floats (`SIM_DOUBLES`), which is what the EE needs. It is switched on by
  `#ifdef DAEDALUS_PSP` in `Core/R4300.cpp:40-44` (the CMake option is never read), next to
  PSP FPU asm at `R4300.cpp:85, 267, 2183, 3307`.

**Memory.**
- RDRAM is always allocated as 8 MB (`Core/Memory.cpp:48`). Dispatch is through two
  tables of 16384 entries indexed by `address >> 18` (256 KB). 8-byte entries make the two
  tables 256 KB. A direct pointer is a fast path; otherwise a handler is called.
- The ROM is either loaded whole (`RomBuffer::Open`, `RomFile/ROMBuffer.cpp:187-319`) or read
  through an LRU chunk cache. Everything uses `spRomData`, so mapping the file in is a
  small change at `ROMBuffer.cpp:211-223` and `:324-328`.
  - In memory the ROM is 32-bit words, little-endian (the `.n64` layout).
  - `PutRomBytesRaw`/`CopyFromRam` can write into the ROM (rare).
- maxsdk has `mx_vm_map_file`: a read-only file in a 64 MiB window (`docs/vm.md`). That is
  exactly the largest N64 cartridge, and clean pages are dropped without I/O.
- PSP budget for comparison: 2 × 3 MB dynarec code buffers
  (`CodeBufferManagerPSP.cpp:141`); the texture cache is purged by age.

**Graphics.**
- A backend implements 7 virtuals of `BaseRenderer` (`HLEGraphics/BaseRenderer.h:231`):
  `TexRect`, `TexRectFlip`, `FillRect`, `Draw2DTexture`, `Draw2DTextureR`,
  `RestoreRenderStates`, `RenderTriangles`. It also needs `CGraphicsContext`,
  `CNativeTexture` and the plugin factory.
- BaseRenderer itself has 36 `DAEDALUS_PSP` ifdefs with direct `sceGu*` calls: viewport,
  scissor, the projection matrix, `TempVerts` from `sceGuGetMemory`, clamp modes and fog.
- Base: the PSP backend (fixed function, like the GS). The GL backend generates GLSL per
  combiner and is no use to libgl.
- Transform and lighting run on the CPU in BaseRenderer. Each VFPU path has a C fallback
  (`DAEDALUS_PSP_USE_VFPU` off). Vertices are submitted in modelview space and the hardware
  applies the projection, which maps onto libgl's `GL_PROJECTION` + VU1.
- Combiner: an automatic analysis (`Combiner/`, ~1.6k lines) plus 122 hand-written modes
  in `BlendModes.cpp` with per-game checks. Texture functions used: MODULATE 62, BLEND 45,
  REPLACE 13, ADD 7, DECAL 6. libgl has MODULATE/REPLACE (and DECAL); **BLEND and ADD are
  missing** (see phase 2).
- Textures: CI4/CI8 stay paletted on PSP, which maps to libgl's
  `GL_OES_compressed_paletted_texture` (T8 with the texture's own CLUT). The other formats
  are converted to 5551/4444/8888 in `HLEGraphics/ConvertImage.cpp`. PSP swizzling and VRAM
  placement are backend-only.
- **No framebuffer readback anywhere.** No render-to-texture, no CFB; CPU-drawn frames
  (`Ucode_FB.h`) are off on PSP. libgl's beam racing loses nothing.

**Platform.**
- Needed: main (`System_Init` → `System_Open(rom)` → `CPU_Run` → `System_Close`), thread,
  timing, mutex/cond, `CInputManager`, `CAudioPlugin`, `CCodeBufferManager`, assert/console,
  and `baseDir`.
- SysCTR is the cleanest self-contained template: its own thread/timing/input/audio, no
  `UI/`. SysPosix/SysPSP take the ROM from `argv`.
- Audio: an RSP audio list runs `Audio_Ucode()` (on the PSP's Media Engine, elsewhere a
  worker thread). The AI buffer is resampled to 44100 Hz stereo s16 and drained by the
  output callback. It is off by default on PSP (speed).
- Toolchain: C++23 and `std::filesystem` (~71 files). Descent 3's Makefile already builds
  libc++/libc++abi on picolibc for `std::filesystem`; here it is the same with `-std=gnu++23`.
- About 125 `DAEDALUS_PSP` ifdefs outside `Sys*/`: HLEGraphics 38, OSHLE 17, UI 14,
  HLEAudio 12, Core 10, RomFile 10.

## Memory plan (32 MiB, VM program)

| What | Size | Where |
|---|---|---|
| Kernel, program code and data | ~4 MB | core |
| RDRAM | 8 MB | core (dynarec loads and stores go to it directly) |
| Memory lookup tables | 256 KB | core |
| Dynarec code buffers | 2 × 2–3 MB | core (executable, flushed) |
| libgl beam racing records | 2 MB | pinned |
| Texture cache (converted textures, EE side) | 1–2 MB | VM heap |
| ROM | up to 64 MB | `mx_vm_map_file`, clean pages, never swapped out |

That leaves a few MB of core for the VM's working set of ROM pages, which N64 games mostly
reach through PI DMA in large blocks: a good access pattern for paging.

## Phases

Test game throughout: Super Mario 64 (from the user's own cartridge dump), then Mario Kart
64, Zelda OoT, GoldenEye and Banjo. Hardware first: PCSX2 runs VM programs only on its
interpreter (slow), and never with more than 32 MB.

### Phase 0: repository and build (S)
- Repository ready: `origin` ps2max32/daedalus (fork), `upstream` DaedalusX64; work on `ps2`, `master` follows upstream.
- `Makefile` on `maxsdk.mk` with `MX_MEMORY=vm`, `MX_KERNEL=1`. Source lists by
  directory, as in `Source/CMakeLists.txt:138-166`.
- libc++ + picolibc from Descent 3's recipe, `-std=gnu++23 -fno-exceptions`.
- Defines: `DAEDALUS_PS2`, the dynarec off (`DynaRec/dummy`), no VFPU, no ME.
- **Done when:** everything but `SysPS2/` compiles, and the link fails only on the platform symbols.

### Phase 1: interpreter, headless (M)
- `Source/SysPS2/`, modelled on SysCTR:
  - main takes the ROM from `argv`, relative paths;
  - threads, timing and mutex on `<mx/thread.h>` and the EE counter;
  - assert/console through `mx_printf`;
  - a null graphics plugin that only counts VI interrupts.
- Core changes:
  - `SIM_DOUBLES` and the FPU rounding code in `R4300.cpp` for `DAEDALUS_PS2`: on the EE,
    the round-to-nearest/floor/ceil conversions in C.
  - `ShouldLoadAsFixed()` true.
  - The ROM through `mx_vm_map_file`, which needs the `.n64` word layout. On first open a
    `.z64`/`.v64` is converted once into `<name>.n64` next to it.
  - A small writable overlay for `PutRomBytesRaw`.
- **Done when:** Mario 64 runs its boot and reaches the title's VI rate on the PS2, prints
  VIs per second and exits cleanly back to the kernel. Measure the interpreter (expect a
  few % of full speed: it is only the reference for phase 5).

### Phase 2: renderer on libgl, SD (L)
- `Source/SysPS2/HLEGraphics/RendererPS2.cpp`, from `RendererPSP.cpp`, with sceGu calls
  mapped to libgl:
  - texture function → `GL_TEXTURE_ENV_MODE`;
  - blend → `glBlendFunc`;
  - alpha test, depth and scissor.
- The BaseRenderer ifdefs get `DAEDALUS_PS2` branches: viewport, scissor, projection via
  `glMatrixMode(GL_PROJECTION)`, `TempVerts` from a plain arena, clamp and fog. Copy
  `BlendModes.cpp` and the Combiner directory from SysPSP; they only call the mapping.
- Textures, `NativeTexturePS2.cpp`:
  - CI4/CI8 → `glCompressedTexImage2D` with `GL_PALETTE4/8_RGBA8_OES`;
  - RGBA16/IA/I → 5551/4444;
  - no swizzle, libgl places textures in GS memory.
- **libgl additions** (useful for every port):
  - `GL_BLEND` texture environment (`Cf·(1−Ct) + Cc·Ct`). On the GS it is two passes:
    first the vertex colour Cf, then constant colour Cc with the texture's intensity as
    alpha (libgl's CLUT already puts intensity in alpha) and blending (Cs − Cd)·As + Cd,
    depth EQUAL. This is exact for I/IA textures, which is how N64 games use BLEND.
  - `GL_ADD` (`Cf + Ct`): a second, additive pass, (Cs − 0)·FIX + Cd.
  - For translucent draws two passes are only approximate. The per-game modes in
    `BlendModes.cpp` take precedence where they exist.
- **Done when:** Mario 64 title, file select and Bob-omb Battlefield match a PC Daedalus
  build's screenshots (`mxgl_screenshot`). A list of the combiner modes that fall back to
  `BlendMode_Generic`, from a debug counter.

### Phase 3: 1080i beam racing (S)
- `mxgl_set_mode(MXGL_1080I | MXGL_BEAM)`, an option at start (`-hd`, like Descent 3's
  `-ps2hd`), and `MXGL_BEAM | MXGL_RGB24` for SD without dithering.
- `MXGL_REC_BYTES` sized from the largest frame record (N64 frames are small; measure).
- The PSP fog pass and the multi-pass texture functions must not rely on `glColorMask`,
  which the beam passes own.
- **Done when:** a 1920 × 1080 screenshot of Mario 64, late passes counted (as in libgl's
  `FPS.md`), steady 60 fields a second at the game's own frame rate.

### Phase 4: input, saves, audio (M)
- Input: pad 1–4 → N64 controllers. The analog left stick goes to the N64 stick with a
  deadzone and a range scale. C buttons on the right stick and/or face buttons, Z on L2/R2.
- Saves: EEPROM, SRAM, FlashRAM and controller paks as files next to the ROM.
- Audio: the HLE audio list in a worker thread at a lower priority, output through maxsdk's
  sound stream at 44.1 kHz (`mx_snd_*`). A setting turns it off (as on PSP, where it costs
  most).
- **Done when:** Mario 64 is playable with sound in the interpreter build (slowly), and a
  save survives a restart.

### Phase 5: R5900 dynarec (L)
- `DynaRec/r5900`: a copy of the `mips` backend with the EE changes, selected for
  `DAEDALUS_PS2`:
  1. `EXT`/`INS` → shift/and/or sequences (5 emit sites, the `.S` stubs).
  2. `clz` in the stubs → a loop or a `PLZCW`-based sequence (PLZCW counts sign bits − 1).
  3. `TRUNC.W.S` → `CVT.W.S`. `FLOOR.W.S`, `ROUND`, `CEIL` and `CVT.W.S` in round-to-nearest
     mode → short sequences (add ±0.5 / compare and adjust) or calls into the stubs.
  4. The icache flush → D-cache writeback (`mx_dcache_writeback`) and `cache 0x0B` (IHIN) per
     64-byte line, then `sync.p`. Patched branches (`AssemblyUtilsPSP.cpp:85-124`) the same.
  5. k0/k1 never allocated. Check the free-register mask in `DynarecTargetPSP.cpp` and
     `Reg_IsTemporary`.
  6. Short-loop erratum: a backward branch over ≤ 6 instructions gets NOPs (as LLVM's
     R5900 delay-slot filler does).
  7. Code buffers in core, 2 × 3 MB first, reduced if the budget needs it.
- Check: run the same ROM for N frames in the interpreter and the dynarec and compare RDRAM
  and register checksums each VI (a debug option of our own).
- **Done when:** Mario 64 matches the interpreter for 3000 frames, and fps is measured in
  SD and 1080i.

### Phase 6: speed (L, open-ended)
- **Native 64-bit operations:** the R5900 has 64-bit GPRs. Generate `daddu`, `dsll32`,
  `ld`/`sd` etc. for the N64's 64-bit operations instead of hi/lo pairs. This is the one
  big advantage over the PSP. Start with the register cache holding a 64-bit value in one
  register.
- **Transform and lighting:** BaseRenderer's C paths with `-mvu0` macro code first. Then
  N64 vertices (G_VTX) transformed and lit on VU1 by libgl, sending modelview-space
  vertices plus light state (the libgl VU1 direction).
- **Texture conversion:** keep the converted texture between frames (the hash check is
  already there) and convert I/IA through CLUTs instead of in software.
- **Audio:** the audio list on a thread of its own; measure what it costs. VU0 microcode
  for the ADPCM/resample loops if it is the limit.
- Profile with the EE counter per subsystem (CPU, RSP gfx, audio, GS wait) on the screen,
  as njemu does.

### Phase 7: front end (S)
- A ROM list (the directory of the ELF, or a path argument), started from mxshell. A pad
  combination brings up options: resolution, audio, frameskip.

## Risks

- **Speed.** The PSP runs a part of the library at full speed with a 333 MHz CPU, the
  VFPU and the Media Engine. We have 294 MHz plus VU0/VU1 and native 64-bit operations.
  Expect a similar set of games, and only after phase 6.
- **The EE FPU is not IEEE** (no NaN, infinity or denormals; clamping). Games that rely on
  them misbehave the same way they do on the PSP.
- **Combiner modes** without a GS equivalent: approximations, per game.
- **C++23 on libc++/picolibc:** any missing header becomes a small shim (as Descent 3 has for
  `std::filesystem`).
