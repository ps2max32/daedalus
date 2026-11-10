# DaedalusX64 on maxsdk: the PS2 port.
#
#   make [OUT=<dir>]     $(OUT)/daedalus.elf
#   make cxxlib          picolibc (with POSIX I/O) and libc++ for this port, once
#
# Source/ is upstream's; Source/SysPS2/ is the platform (DAEDALUS_PS2), with
# the 3DS backend's GL 1.x renderer on libs/libgl. Upstream builds with CMake;
# the source lists here follow Source/*/CMakeLists.txt.

HERE     := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PS2MAX   ?= $(abspath $(HERE)/../..)
MAXSDK   ?= $(PS2MAX)/libs/maxsdk
MAXSDK_OUT ?= $(PS2MAX)/build/libs/maxsdk
LIBOUT   := $(PS2MAX)/build/apps/daedalus
# the emulator in virtual memory (the ROM is a file mapped into it), its code in core
MX_MEMORY ?= vm
MX_VM_TEXT ?= core
MX_KERNEL ?= 1
OUT      ?= $(LIBOUT)
all: $(OUT)/daedalus.elf

MXGL     ?= $(PS2MAX)/libs/libgl
include $(MXGL)/mk/mxgl.mk
include $(MAXSDK)/mk/maxsdk.mk
MX_CORE_OBJS = $(MXGL_OBJS)

LLVM_SRC ?= $(PS2MAX_LLVM_SRC)
PICOLIBC_SRC ?= $(PS2MAX)/libs/picolibc
PICOLIBC := $(LIBOUT)/picolibc/install
LIBCXX   := $(LIBOUT)/libcxx/install
PICOLIBC_A := $(PICOLIBC)/lib/libc.a
LIBCXX_A   := $(LIBCXX)/lib/libc++.a
CXX      := $(LLVM)/clang++

LIBRT    = $(MAXSDK_OUT)/librt.a
# libmxc's malloc (TLSF): picolibc's clears what it hands out and walks the heap on free
LIBMXC_OBJS = $(OUT)/libc/glue.o $(MAXSDK_OUT)/libc/compat.c.o $(MAXSDK_OUT)/libc/atomic.c.o \
              $(MAXSDK_OUT)/libc/malloc.c.o
LIBC_LIBS = $(LIBMXC_OBJS) $(LIBCXX)/lib/libc++.a $(LIBCXX)/lib/libc++abi.a $(PICOLIBC_A) \
            $(MX_LIB) $(LIBRT)

S        := $(HERE)/Source
P        := $(S)/SysPS2
TARGETF  := --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G0
SYSINC   := -isystem $(P)/include -isystem $(PICOLIBC)/include
LIB_FLAGS := $(TARGETF) -nostdlibinc $(SYSINC) -ffunction-sections -fdata-sections -D_GNU_SOURCE \
             -D_POSIX_TIMERS=1 -D_POSIX_MONOTONIC_CLOCK=200112L -Wno-c++11-narrowing

OPT      ?= -O2
# the options upstream's CMake gives a PSP build, without the PSP's hardware
# (Media Engine, VFPU) and with the dynarec off (phase 5)
DEFS     := -DDAEDALUS_PS2 -DDAEDALUS_ENDIAN_LITTLE -DDAEDALUS_SIM_DOUBLES -DDAEDALUS_SILENT \
            -DDAEDALUS_ENABLE_OS_HOOKS -D_USE_MATH_DEFINES -DDAEDALUS_CONFIG_VERSION='"PS2"' \
            -D_GNU_SOURCE -DNDEBUG $(EXTRA_DEFS)
COMMON   := $(TARGETF) $(OPT) -fno-common -ffunction-sections -fdata-sections -MMD \
            -fno-strict-aliasing -ffinite-math-only -fno-math-errno -ffast-math -mvu0 \
            -Wno-multichar -Wno-c++11-narrowing $(DEFS) -I$(S) -I$(MXGL_INC) -I$(MAXSDK)/ee/include
DCXXFLAGS := $(COMMON) -std=gnu++23 -fno-exceptions -nostdinc++ -nostdlibinc -include ps2_port.h \
             -isystem $(LIBCXX)/include/c++/v1 $(SYSINC)
DCFLAGS  := $(COMMON) -std=gnu11 -nostdlibinc $(SYSINC)

# ---- sources (Source/*/CMakeLists.txt) ---------------------------------------
CORE     := $(addprefix Core/,CPU DMA Dynamo FlashMem Interpret Interrupt Memory PIF R4300 RDRam \
              ROM ROMImage RSP_HLE Save TLB)
DEBUG    := $(addprefix Debug/,DebugLog Dump PrintOpCode Registers)
DYNAREC  := $(addprefix DynaRec/,BranchType DynaRecProfile Fragment FragmentCache IndirectExitMap \
              StaticAnalysis TraceRecorder dummy/CodeBufferManagerDummy dummy/AssemblyUtilsDummy)
GRAPHICS := $(addprefix Graphics/,ColourValue HvqmTask JpegTask TextureTransform)
HLEAUDIO := $(addprefix HLEAudio/,ABI1 ABI2 ABI3 ABI3mp3 AudioBuffer ABI_ADPCM ABI_Buffers \
              ABI_Filters ABI_MixerInterleave ABI_Resample ENV_Mixer HLEAudioState HLEMain)
HLEGFX   := $(addprefix HLEGraphics/,BaseRenderer CachedTexture ConvertImage ConvertTile DLDebug \
              DLParser Microcode RDP RDPStateManager TextureCache TextureInfo TMEM uCodes/Ucode)
IFACE    := $(addprefix Interface/,ConfigOptions Cheats Preferences RomDB SaveState)
OSHLE    := $(addprefix OSHLE/,OS patch)
ROMFILE  := $(addprefix RomFile/,RomFile RomFileCache RomFileCompressed RomFileMemory \
              RomFileUncompressed ROMBuffer RomSettings)
SYSTEM   := $(addprefix System/,CompressedStream SystemInit)
UTILITY  := $(addprefix Utility/,CRC FastMemcpy FramerateLimiter Hash IniFile MemoryHeap Paths \
              Profiler Stream StringUtil Timer Translate)
UPSTREAM := $(CORE) $(DEBUG) $(DYNAREC) $(GRAPHICS) $(HLEAUDIO) $(HLEGFX) $(IFACE) $(OSHLE) \
            $(ROMFILE) $(SYSTEM) $(UTILITY)
# the platform: Source/SysPS2 and the PS2 directories next to the other platforms'
PS2_CXX  := $(patsubst $(S)/%.cpp,%,$(wildcard $(P)/*.cpp $(P)/*/*.cpp $(P)/*/*/*.cpp)) \
            System/Thread/PS2/Thread System/Timing/PS2/Timing Input/PS2/InputManagerPS2 \
            HLEAudio/Plugin/PS2/AudioPluginPS2
PS2_C    := $(patsubst $(S)/%.c,%,$(wildcard $(P)/*.c $(P)/*/*.c))
ZLIB_SRC ?= $(PS2MAX)/libs/zlib
ZLIB_C   := adler32 crc32 inffast inflate inftrees uncompr zutil deflate trees

UP_OBJS  := $(patsubst %,$(OUT)/o/%.o,$(UPSTREAM))
PS2_OBJS := $(patsubst %,$(OUT)/o/%.o,$(PS2_CXX)) $(patsubst %,$(OUT)/o/%.c.o,$(PS2_C))
OBJS     := $(UP_OBJS) $(PS2_OBJS) $(patsubst %,$(OUT)/zlib/%.o,$(ZLIB_C))

CXXDEPS  := $(LIBCXX_A) $(HERE)/Makefile
$(OUT)/o/%.o: $(S)/%.cpp $(CXXDEPS)
	@mkdir -p $(dir $@)
	$(CXX) $(DCXXFLAGS) -I$(ZLIB_SRC) -c $< -o $@
$(OUT)/o/%.c.o: $(S)/%.c $(PICOLIBC_A)
	@mkdir -p $(dir $@)
	$(CC) $(DCFLAGS) -c $< -o $@
$(OUT)/zlib/%.o: $(ZLIB_SRC)/%.c $(PICOLIBC_A)
	@mkdir -p $(dir $@)
	$(CC) $(DCFLAGS) -c $< -o $@

# phase 0: everything upstream compiles (make upstream)
upstream: $(UP_OBJS)

# maxsdk's libc glue with read, write and _exit renamed: SysPS2/ps2_posix.c has
# them (files) and passes the console's on
$(OUT)/libc/glue.o: $(MAXSDK)/ee/libc/glue.c $(PICOLIBC_A) $(HERE)/Makefile
	@mkdir -p $(dir $@)
	$(CC) $(filter-out -ffreestanding,$(CFLAGS)) -nostdlibinc $(SYSINC) -Dread=mx_glue_read \
	  -Dwrite=mx_glue_write -D_exit=mx_glue_exit -c $< -o $@

$(OUT)/daedalus.elf: $(OBJS) $(MXGL_OBJS) $(MX_LIB) $(MX_CRT0) $(MX_LD) $(LIBC_LIBS)
	$(call link,$@,--error-limit=0 --defsym=__mx_stack_size=0x40000 $(MX_CRT0) $(OBJS) \
	  $(MXGL_OBJS) --start-group $(LIBC_LIBS) --end-group)

# ---- picolibc and libc++ (as apps/descent3) -----------------------------------
cxxlib: $(LIBCXX_A)
$(LIBCXX)/lib/libc++abi.a: $(LIBCXX_A) ;
$(MXGL_OBJS): $(PICOLIBC_A)

$(PICOLIBC_A):
	@mkdir -p $(LIBOUT)/picolibc
	sed "s|@LLVM@|$(LLVM)|g" $(MAXSDK)/ee/libc/cross-r5900.txt.in > $(LIBOUT)/picolibc/cross.txt
	meson setup --wipe $(LIBOUT)/picolibc/build-meson $(PICOLIBC_SRC) \
	  --cross-file $(LIBOUT)/picolibc/cross.txt --prefix=$(PICOLIBC) -Dmultilib=false \
	  -Dpicocrt=false -Dsemihost=false -Dtests=false -Dthread-local-storage=false \
	  -Dposix-io=true -Dspecsdir=none -Dio-long-long=true -Dbuildtype=release
	ninja -C $(LIBOUT)/picolibc/build-meson install

$(LIBCXX_A): $(PICOLIBC_A)
	cmake -G Ninja -S $(LLVM_SRC)/runtimes -B $(LIBOUT)/libcxx/build \
	  -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_INSTALL_PREFIX=$(LIBCXX) -DCMAKE_C_COMPILER=$(LLVM)/clang \
	  -DCMAKE_CXX_COMPILER=$(LLVM)/clang++ -DCMAKE_AR=$(LLVM)/llvm-ar \
	  -DCMAKE_RANLIB=$(LLVM)/llvm-ranlib -DCMAKE_C_FLAGS="$(LIB_FLAGS)" \
	  -DCMAKE_CXX_FLAGS="$(LIB_FLAGS)" -DCMAKE_SYSTEM_NAME=Generic \
	  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DRUNTIMES_USE_LIBC=picolibc \
	  -DLIBCXXABI_BAREMETAL=ON -DLIBCXXABI_ENABLE_ASSERTIONS=OFF \
	  -DLIBCXXABI_ENABLE_EXCEPTIONS=OFF -DLIBCXXABI_ENABLE_SHARED=OFF \
	  -DLIBCXXABI_ENABLE_THREADS=OFF -DLIBCXXABI_USE_LLVM_UNWINDER=OFF \
	  -DLIBCXXABI_USE_COMPILER_RT=ON -DLIBCXXABI_INCLUDE_TESTS=OFF \
	  -DLIBCXX_ENABLE_EXCEPTIONS=OFF -DLIBCXX_ENABLE_FILESYSTEM=ON \
	  -DLIBCXX_ENABLE_MONOTONIC_CLOCK=ON -DLIBCXX_ENABLE_RANDOM_DEVICE=OFF \
	  -DLIBCXX_ENABLE_RTTI=ON -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_ENABLE_THREADS=OFF \
	  -DLIBCXX_ENABLE_WIDE_CHARACTERS=OFF -DLIBCXX_ENABLE_LOCALIZATION=ON \
	  -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
	  -DLIBCXX_INCLUDE_TESTS=OFF -DLIBCXX_USE_COMPILER_RT=ON -DLIBCXX_CXX_ABI=libcxxabi \
	  -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON -DLIBCXX_SHARED_OUTPUT_NAME=c++-shared \
	  -DLIBCXXABI_SHARED_OUTPUT_NAME=c++abi-shared
	ninja -C $(LIBOUT)/libcxx/build install-cxx install-cxxabi

clean:
	rm -rf $(OUT)/o $(OUT)/zlib $(OUT)/libc $(OUT)/*.elf $(OUT)/*.map

.PHONY: all cxxlib upstream clean
-include $(shell find $(OUT)/o -name '*.d' 2>/dev/null)
