# ScummVM port to openfpgaOS (Analogue Pocket) -- per-engine multi-ELF build.
#
# Compiles the engine-agnostic ScummVM CORE once into build-multi/libcore.a,
# then links one self-contained scummvm_<engine>.elf per engine, all sharing
# that core. The shipped engines are SCUMM (label "lucasarts"), AGI and SCI;
# each game instance's os.ini points [os] ELF= at the ELF for its engineid.
#
#   make                 build all engine ELFs + assemble build/<target>/scummvm/
#   make copy            build, assemble, copy to the SD card
#   make package         build, assemble, zip into releases/<target>/
#   make debug           build, push the lucasarts ELF over UART, stream console
#   make clean           remove build-multi/ and the assembled image
#   make ENGINES=scumm   build a subset (default: scumm agi sci dreamweb)
#
# Statically links upstream musl (SDK $(SDK_DIR)/musl/). Per-engine selection is
# via -DENABLE_<X> + a generated plugins/detection table, independent of the
# ENABLE_* macros in config.h (the core strips ENABLE_SCUMM from a config shadow).

ENGINES ?= scumm agi sci dreamweb

# Utility/pattern rules precede the phony targets, so pin the default goal.
.DEFAULT_GOAL := all

# ── Per-engine source curation ──────────────────────────────────────
# Feature-gated files whose feature is OFF in this port (mirrors each engine's
# module.mk ifdef blocks). SCI32 is now ENABLED and renders in CLUT8 (8-bit, no
# RGB): the SCI32 TUs are compiled in (EXCLUDE_sci empty) and -DENABLE_SCI32 is
# added to the sci ELF (EXTRA_DEFS_sci) and to CORE_ENGINE_FLAGS.
EXCLUDE_sci =

# Output label per engine (default = engine id). SCUMM ships as "lucasarts".
LABEL_scumm = lucasarts

# Extra per-engine compile defines, appended to ECXX_<id> below. SCI adds
# -DENABLE_SCI32 (the CLUT8 SCI32 set) on top of the generic -DENABLE_SCI;
# scoped to the sci ELF only -- UC_sci=SCI yields -DENABLE_SCI, never SCI32.
EXTRA_DEFS_sci = -DENABLE_SCI32

# SCUMM needs a curated source set, not a naive glob (engines/scumm/ pulls all of
# he/ = Humongous, which needs ENABLE_HE). Mirror the classic set: drop the
# v7/v8 SMUSH bits, take only the few he/ files base SCUMM needs. An engine with
# SRCS_<id> defined uses it verbatim instead of the wildcard.
SCUMM_EXCLUDE = %/dialog-createsession.cpp %/dialog-sessionselector.cpp %/nut_renderer.cpp %/string_v7.cpp %/script_v8.cpp
SRCS_scumm = $(filter-out $(SCUMM_EXCLUDE), $(wildcard $(SCUMMVM)/engines/scumm/*.cpp)) \
	$(wildcard $(SCUMMVM)/engines/scumm/imuse/*.cpp) \
	$(wildcard $(SCUMMVM)/engines/scumm/imuse/drivers/*.cpp) \
	$(wildcard $(SCUMMVM)/engines/scumm/players/*.cpp) \
	$(wildcard $(SCUMMVM)/engines/scumm/macgui/*.cpp) \
	$(SCUMMVM)/engines/scumm/he/mixer_he.cpp \
	$(SCUMMVM)/engines/scumm/he/resource_he.cpp \
	$(SCUMMVM)/engines/scumm/he/script_v60he.cpp \
	$(SCUMMVM)/engines/scumm/he/script_v70he.cpp \
	$(SCUMMVM)/engines/scumm/he/sound_he.cpp

# ── Toolchain ───────────────────────────────────────────────────────
CROSS ?= $(shell which riscv64-unknown-elf-gcc >/dev/null 2>&1 && echo riscv64-unknown-elf- || echo riscv64-elf-)
CC   = $(CROSS)gcc
CXX  = $(CROSS)g++
AR   = $(CROSS)ar
SIZE = $(CROSS)size

ARCH = rv32imafc
ABI  = ilp32f

SDK_DIR  = ../sdk
MUSL_DIR = $(SDK_DIR)/musl
SCUMMVM  = scummvm
APP_LD   = $(SDK_DIR)/app.ld
CRT_OBJS = $(MUSL_DIR)/lib/crt1.o $(MUSL_DIR)/lib/crti.o $(MUSL_DIR)/lib/crtn.o

BUILD    = build-multi
GENDIR   = $(BUILD)/gen
LIBCORE  = $(BUILD)/libcore.a

# ── Release / deploy paths ──────────────────────────────────────────
APP       = scummvm
TARGET   ?= pocket
MISTER_IP ?= mister.local
ROOT      = $(realpath $(CURDIR)/../..)
DIST      = $(ROOT)/dist/$(APP)
OUT       = $(ROOT)/build/$(TARGET)/$(APP)
COMMON    = $(OUT)/Assets/$(APP)/common
RUNTIME   = $(ROOT)/runtime
# Optional user-supplied game data staged beside this Makefile. The DreamWeb
# packaging helper produces dreamweb.zip here; it is intentionally untracked.
GAME_ASSETS = $(wildcard *.dat *.bin *.cfg *.zip)

# ── Flags ───────────────────────────────────────────────────────────
COMMON_FLAGS  = -march=$(ARCH) -mabi=$(ABI) -O2 -Wall
COMMON_FLAGS += -ffunction-sections -fdata-sections
COMMON_FLAGS += -fno-tree-loop-distribute-patterns
COMMON_FLAGS += -nostdinc -I$(SDK_DIR)/include -isystem $(MUSL_DIR)/include -I.
# 24 soft MIDI voices (SDK default 12).  Dense SCI scores + per-note stereo
# zone layers + release tails saturate 12 immediately, forcing audible voice
# steals on every rapid note.  HW has 31 usable mixer voices and ScummVM's
# digital audio uses the stream API (not HW voices), so MUSIC can own 24.
# Costs ~1.5 KB extra BRAM (.app_fastdata) and ~2x worst-case 1 kHz tick.
COMMON_FLAGS += -DSMP_MAX_VOICES=24

CFLAGS = $(COMMON_FLAGS)

CXXFLAGS  = $(COMMON_FLAGS)
CXXFLAGS += -std=gnu++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics
CXXFLAGS += -Icompat/cxx_includes
CXXFLAGS += -DNONSTANDARD_PORT -DHAVE_CONFIG_H -DDISABLE_COMMAND_LINE
# Upstream's shipping configuration: compiles out the #ifndef RELEASE_BUILD
# blocks -- notably the per-pixel _rowEdge asserts in celobj32's SCALER
# read() paths, which otherwise cost a live compare+branch on every pixel
# of every generic cel draw.  Plain assert() stays live (no -DNDEBUG): the
# save/audio layers rely on those sanity checks to catch corruption early.
CXXFLAGS += -DRELEASE_BUILD
CXXFLAGS += -Wno-reorder -Wno-unused-parameter -Wno-sign-compare
CXXFLAGS += -Wno-missing-field-initializers -Wno-unused-variable
CXXFLAGS += -Wno-unknown-pragmas -Wno-multichar
CXXFLAGS += -I$(SCUMMVM) -I$(SCUMMVM)/engines -I$(SCUMMVM)/backends

LDFLAGS  = -march=$(ARCH) -mabi=$(ABI) -nostdlib -static
LDFLAGS += -T $(APP_LD) -L$(MUSL_DIR)/lib
LDFLAGS += -Wl,--gc-sections -Wl,--no-warn-rwx-segments
LIBGCC = $(shell $(CC) -march=$(ARCH) -mabi=$(ABI) -print-libgcc-file-name)
LIBS   = -Wl,--start-group -lc -lm -lgcc -Wl,--end-group

# Core config = port config.h WITHOUT the hardcoded ENABLE_SCUMM, so the engine
# is selected per-ELF via -DENABLE_<X>. Found ahead of ./config.h via -I$(GENDIR).
CONFIG_SHADOW = $(GENDIR)/config.h

# The core sees the UNION of our engines' flags so engine-gated CORE code is
# built once (e.g. Graphics::FontSJIS, used by SCUMM/SCI). Unused gated code is
# trimmed per-ELF by --gc-sections.
# ENABLE_SCI32 must be in the CORE union too: video/coktel_decoder.cpp (libcore)
# is gated on it, and sci's video32.o needs its VMD/IMD decoders at link time.
CORE_ENGINE_FLAGS = -DENABLE_SCUMM -DENABLE_SCI -DENABLE_SCI32 -DENABLE_AGI -DENABLE_DREAMWEB
CORE_CXXFLAGS = -I$(GENDIR) $(CXXFLAGS) $(CORE_ENGINE_FLAGS)

# src path -> object path under $(BUILD)/obj  (../sdk/foo -> up/sdk/foo)
to_obj = $(foreach s,$(1),$(BUILD)/obj/$(patsubst ../%,up/%,$(basename $(s))).o)

# ── Engine-agnostic CORE sources (compiled once into libcore.a) ─────
# NB: base/plugins.cpp and the local main.cpp are PER-ENGINE (below): plugins.cpp
# #includes the per-engine engines/plugins_table.h, and main.cpp takes the
# per-engine default-engine macro.
CORE_COMMON = $(filter-out %/recorderfile.cpp %/updates.cpp, $(wildcard $(SCUMMVM)/common/*.cpp)) \
	$(wildcard $(SCUMMVM)/common/encodings/*.cpp) \
	$(wildcard $(SCUMMVM)/common/formats/*.cpp) \
	$(filter-out %/zlib.cpp, $(wildcard $(SCUMMVM)/common/compression/*.cpp)) \
	$(wildcard $(SCUMMVM)/common/std/*.cpp)

CORE_BASE = $(SCUMMVM)/base/main.cpp $(SCUMMVM)/base/version.cpp $(SCUMMVM)/base/commandLine.cpp

CORE_GRAPHICS = $(filter-out %/svg.cpp %/larryScale_generated.cpp, $(wildcard $(SCUMMVM)/graphics/*.cpp)) \
	$(filter-out %/blit-atari.cpp %/blit-neon.cpp %/blit-sse2.cpp %/blit-avx2.cpp, $(wildcard $(SCUMMVM)/graphics/blit/*.cpp)) \
	$(wildcard $(SCUMMVM)/graphics/fonts/*.cpp) \
	$(wildcard $(SCUMMVM)/graphics/macgui/*.cpp) \
	$(wildcard $(SCUMMVM)/graphics/scaler/*.cpp)

AUDIO_EXCLUDE = %/mp3.cpp %/vorbis.cpp %/flac.cpp %/aac.cpp %/ac3.cpp %/alsa_opl.cpp %/opl2lpt.cpp %/retrowave_opl3.cpp %/rwopl3.cpp %/fluidsynth.cpp %/mt32.cpp %/eas.cpp
CORE_AUDIO = $(filter-out $(AUDIO_EXCLUDE), \
	$(wildcard $(SCUMMVM)/audio/*.cpp) $(wildcard $(SCUMMVM)/audio/decoders/*.cpp) \
	$(wildcard $(SCUMMVM)/audio/softsynth/*.cpp) $(wildcard $(SCUMMVM)/audio/softsynth/opl/*.cpp) \
	$(wildcard $(SCUMMVM)/audio/softsynth/fmtowns_pc98/*.cpp) $(wildcard $(SCUMMVM)/audio/mods/*.cpp))

CORE_ENGINES_FW = $(wildcard $(SCUMMVM)/engines/*.cpp)

CORE_BACKENDS = \
	$(SCUMMVM)/backends/base-backend.cpp $(SCUMMVM)/backends/modular-backend.cpp \
	$(SCUMMVM)/backends/mixer/null/null-mixer.cpp \
	$(SCUMMVM)/backends/timer/default/default-timer.cpp \
	$(SCUMMVM)/backends/events/default/default-events.cpp \
	$(SCUMMVM)/backends/saves/default/default-saves.cpp \
	$(SCUMMVM)/backends/saves/savefile.cpp \
	$(SCUMMVM)/backends/audiocd/default/default-audiocd.cpp \
	$(wildcard $(SCUMMVM)/backends/keymapper/*.cpp) \
	$(SCUMMVM)/backends/fs/abstract-fs.cpp

GUI_EXCLUDE = %/onscreendialog.cpp %/recorderdialog.cpp %/updates-dialog.cpp %/EventRecorder.cpp %/editrecorddialog.cpp %/fluidsynth-dialog.cpp %/dump-all-dialogs.cpp %/integrity-dialog.cpp
CORE_GUI = $(filter-out $(GUI_EXCLUDE), $(wildcard $(SCUMMVM)/gui/*.cpp) $(wildcard $(SCUMMVM)/gui/widgets/*.cpp))

CORE_IMAGE = $(filter-out %/gif.cpp %/mpeg.cpp, $(wildcard $(SCUMMVM)/image/*.cpp) $(wildcard $(SCUMMVM)/image/codecs/*.cpp))

VIDEO_EXCLUDE = %/theora_decoder.cpp %/bink_decoder.cpp %/hnm_decoder.cpp %/mpegps_decoder.cpp %/psx_decoder.cpp %/mkv_decoder.cpp
CORE_VIDEO = $(filter-out $(VIDEO_EXCLUDE), $(wildcard $(SCUMMVM)/video/*.cpp))

CORE_MATH = $(wildcard $(SCUMMVM)/math/*.cpp)

CORE_SRCS = $(CORE_COMMON) $(CORE_BASE) $(CORE_GRAPHICS) $(CORE_AUDIO) $(CORE_ENGINES_FW) \
	$(CORE_BACKENDS) $(CORE_GUI) $(CORE_IMAGE) $(CORE_VIDEO) $(CORE_MATH)
CORE_OBJS = $(call to_obj,$(CORE_SRCS))

# Local backend + SDK runtime: engine-agnostic, but linked DIRECTLY into each
# ELF (not archived) since some are reached only via constructors / the SDK.
DIRECT_CPP = $(SDK_DIR)/of_cxxabi.cpp \
	backend/openfpga_osystem.cpp backend/openfpga_memory.cpp backend/openfpga_mixer.cpp \
	backend/openfpga_midi.cpp backend/openfpga_save.cpp backend/openfpga_fs.cpp \
	backend/openfpga_audiocd.cpp backend/openfpga_cue_archive.cpp compat/zlib_stub.cpp
DIRECT_C = $(SDK_DIR)/of_init.c $(SDK_DIR)/of_midi.c $(SDK_DIR)/of_smp_bank.c \
	$(SDK_DIR)/of_smp_tables.c $(SDK_DIR)/of_smp_voice.c
DIRECT_OBJS = $(call to_obj,$(DIRECT_CPP)) $(call to_obj,$(DIRECT_C))

# ── Compile rules (objects under $(BUILD)/obj, paths mirrored) ───────
# -MMD -MP: emit header dependency files next to each object and include
# them below, so editing a HEADER rebuilds every object that includes it.
# Without this, a change to a class layout (e.g. adding a ScummEngine
# member in scumm.h) rebuilt only the touched .cpp files and linked
# MIXED-LAYOUT objects -- stale code read members at old offsets and
# crashed at boot in ways no source read could explain (2026-07-18:
# initVirtScreen height assert from a stale gfx.o reading _screenHeight).
DEPFLAGS = -MMD -MP
$(BUILD)/obj/up/%.o: ../%.cpp | $(CONFIG_SHADOW)
	@mkdir -p $(dir $@)
	$(CXX) $(CORE_CXXFLAGS) $(DEPFLAGS) -c -o $@ $<
$(BUILD)/obj/up/%.o: ../%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<
$(BUILD)/obj/%.o: %.cpp | $(CONFIG_SHADOW)
	@mkdir -p $(dir $@)
	$(CXX) $(CORE_CXXFLAGS) $(DEPFLAGS) -c -o $@ $<
$(BUILD)/obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

$(CONFIG_SHADOW): config.h
	@mkdir -p $(dir $@)
	@grep -v 'ENABLE_SCUMM' config.h > $@
	@echo "  [gen] $@  (core config; ENABLE_SCUMM stripped)"

$(LIBCORE): $(CORE_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(CORE_OBJS)
	@echo "  [ar]  $@  ($(words $(CORE_OBJS)) objects)"

# ── Per-engine instantiation ────────────────────────────────────────
define ENGINE_template
UC_$(1)    := $(shell echo $(1) | tr a-z A-Z)
OUT_$(1)   := $$(if $$(LABEL_$(1)),$$(LABEL_$(1)),$(1))
GEN_$(1)   := $(BUILD)/$(1)/gen/.tables.stamp
ESRC_$(1)  := $$(filter-out $$(EXCLUDE_$(1)), $(SCUMMVM)/base/plugins.cpp main.cpp \
	$$(if $$(SRCS_$(1)),$$(SRCS_$(1)),$$(wildcard $(SCUMMVM)/engines/$(1)/*.cpp $(SCUMMVM)/engines/$(1)/*/*.cpp $(SCUMMVM)/engines/$(1)/*/*/*.cpp)))
EOBJ_$(1)  := $$(foreach s,$$(ESRC_$(1)),$(BUILD)/$(1)/obj/$$(basename $$(s)).o)
ECXX_$(1)  := -I$(BUILD)/$(1)/gen -I$(GENDIR) $(CXXFLAGS) -DENABLE_$$(UC_$(1)) $$(EXTRA_DEFS_$(1)) -DOPENFPGA_DEFAULT_ENGINE='"$(1)"'

# Generate the engine + detection plugin tables (DETECTION_STATIC is on, so
# plugins.cpp pulls both; each engine's own detection.cpp supplies the symbol).
# A single stamp gates BOTH headers so no engine object compiles before both
# exist (avoids a -j race where plugins.o picks up the real SCUMM table).
$$(GEN_$(1)):
	@mkdir -p $(BUILD)/$(1)/gen/engines
	@echo 'LINK_PLUGIN($$(UC_$(1)))' > $(BUILD)/$(1)/gen/engines/plugins_table.h
	@echo 'LINK_PLUGIN($$(UC_$(1))_DETECTION)' > $(BUILD)/$(1)/gen/engines/detection_table.h
	@touch $$@
	@echo "  [gen] $(1): plugins_table.h + detection_table.h"

$$(EOBJ_$(1)): $(BUILD)/$(1)/obj/%.o: %.cpp $$(GEN_$(1)) | $(CONFIG_SHADOW)
	@mkdir -p $$(dir $$@)
	$(CXX) $$(ECXX_$(1)) $(DEPFLAGS) -c -o $$@ $$<

$(BUILD)/scummvm_$$(OUT_$(1)).elf: $$(EOBJ_$(1)) $(DIRECT_OBJS) $(LIBCORE) $(CRT_OBJS) $(APP_LD)
	$(CXX) $(LDFLAGS) -o $$@ $(CRT_OBJS) $$(EOBJ_$(1)) $(DIRECT_OBJS) $(LIBCORE) $(LIBS) $(LIBGCC)
	@echo "== scummvm_$$(OUT_$(1)).elf (engine: $(1)) =="; $(SIZE) $$@
endef
$(foreach E,$(ENGINES),$(eval $(call ENGINE_template,$(E))))

ENGINE_ELFS = $(foreach E,$(ENGINES),$(BUILD)/scummvm_$(OUT_$(E)).elf)

# ── Targets ─────────────────────────────────────────────────────────
.PHONY: all build release copy package debug clean game-dist

# `make` builds every engine ELF, then assembles the SD-card image.
all: release

# ── Container-by-default RISC-V build (mirrors sdk.mk) ──────────────
# This Makefile rolls its own compile rules (no sdk.mk include), so it
# carries the same delegate guard: outside the container `build` re-execs
# ONE `make build` inside the openfpgaos-firmware image, where the
# toolchain is pinned to riscv64-unknown-elf-.  Host-side targets
# (release/copy/package — image assembly) keep running on the host.
# USE_SDK_CONTAINER=0 is the explicit host-toolchain opt-out.
USE_SDK_CONTAINER ?= 1
SDK_CONTAINER    ?= $(ROOT)/tools/sdk-container.sh
ifeq ($(USE_SDK_CONTAINER)_$(OF_SDK_IN_CONTAINER),1_)

build: SDK_FORCE
	@bash $(SDK_CONTAINER) make build ENGINES="$(ENGINES)"
SDK_FORCE:
.PHONY: SDK_FORCE

else

build: $(ENGINE_ELFS)
	@echo; echo "Built per-engine ELFs sharing one libcore.a:"
	@for f in $(notdir $(ENGINE_ELFS)); do echo "  $(BUILD)/$$f"; done

endif

# Assemble the deployable tree: image.sh wipes+rebuilds $(OUT) from dist/
# (instance os.ini/JSON already point at the per-engine ELFs), then we drop ALL
# engine ELFs into common/ and remove the legacy single-ELF seed image.sh makes.
#
# MiSTer has no image.sh — its deliverable is the per-game bundle (game-dist
# below), so `release` branches on TARGET.  Without this guard a mister build
# links fine and then dies with exit 127 on the missing platforms/mister/image.sh.
ifeq ($(TARGET),mister)
release: game-dist        # MiSTer: the per-game bundle IS the release artifact
else
release: build
	@VARIANT="$(VARIANT)" $(SDK_DIR)/platforms/$(TARGET)/image.sh "$(APP)" "$(firstword $(ENGINE_ELFS))" "$(ROOT)" "$(DIST)" $(GAME_ASSETS)
	@cp $(ENGINE_ELFS) "$(COMMON)/"
	@rm -f "$(COMMON)/$(APP).elf"
	@echo "   engine ELFs in common/: $(notdir $(ENGINE_ELFS))"
endif

# MiSTer engine update: every instance F-loads the ELF its ini names, and this
# port ships ONE ELF PER ENGINE, so a per-game engine update pushes them all as
# loose games/OpenfpgaOS/ScummVM/<engine>.elf files — boot.vhd (the user's game
# data) and the saves volume are untouched.
copy: all
ifeq ($(TARGET),mister)
	@for e in $(ENGINE_ELFS); do \
	    MISTER_IP="$(MISTER_IP)" $(SDK_DIR)/platforms/mister/copy.sh \
	        game "$(GAME)" "$$(basename $$e)" "$$e" || exit 1; \
	done
else
	@MISTER_IP="$(MISTER_IP)" $(SDK_DIR)/platforms/$(TARGET)/copy.sh "$(APP)" "$(firstword $(ENGINE_ELFS))"
endif

# `make package` for MiSTer builds the per-game locked distribution (a boot.vhd
# shell + .mgl launchers + loose inis + setup.sh + external_files.csv, zipped for
# games/OpenfpgaOS/); every other target keeps the single-image path.
# scripts/package.sh -> platforms/<target>/package.sh auto-detects which.
#
# For MiSTer it ALSO generates the Downloader custom-database delivery artifact
# (releases/mister/scummvm.json.zip) via mkdb.py — a real ZIP-wrapped JSON the
# stock MiSTer Downloader syncs.  external_files.csv is folded into it here at
# build time (it is NOT a device file).  db_id must never change once published.
#
# Publishing model (edit these for the real release):
#   MISTER_DB_ID       downloader.ini [section] == db_id — unique to THIS game
#   MISTER_DB_BASE_URL where the STAGED tree is hosted; a local file at sd path
#                      <p> resolves to  $(MISTER_DB_BASE_URL)<p>  (base_files_url
#                      + path — the Downloader's own rule).  Host the game-dist
#                      tree under games/OpenfpgaOS/ at this base.
#   MISTER_DB_URL      where scummvm.json.zip itself is published (ini snippet)
# Owner tracks the actual remote: github.com/openfpgaOS/ScummVM.  The old
# thinkelastic/ owner was stale, so every generated downloader.ini pointed at
# a 404.
MISTER_DB_ID       ?= openfpgaOS/openfpgaos-scummvm
MISTER_DB_BASE_URL ?= https://raw.githubusercontent.com/openfpgaOS/ScummVM/mister-dist/
MISTER_DB_URL      ?= https://raw.githubusercontent.com/openfpgaOS/ScummVM/mister-dist/scummvm.json.zip
MISTER_DB          = $(ROOT)/releases/mister/$(APP).json.zip
MISTER_DB_INI      = $(ROOT)/releases/mister/$(APP).downloader.ini
ifeq ($(TARGET),mister)
package: game-dist
	@$(ROOT)/scripts/package.sh $(APP)
	@$(SDK_DIR)/platforms/mister/mkdb.py \
	    --staging "$(GAME_STAGE)" --sd-prefix "games/OpenfpgaOS" \
	    --menu-dir "_Computer/_OpenfpgaOS/_$(GAME)" \
	    --base-url "$(MISTER_DB_BASE_URL)" --db-id "$(MISTER_DB_ID)" \
	    --external "$(GAME_CSV)" --db-url "$(MISTER_DB_URL)" \
	    --output "$(MISTER_DB)" --ini-out "$(MISTER_DB_INI)"
else
package: all
	@$(ROOT)/scripts/package.sh $(APP)
endif

# ── MiSTer per-game locked distribution (S0 boot shell + S1 saves + .mgl) ─
# The core (openfpgaOS.rbf + os.bin) is game-agnostic.  ScummVM ships a pair of
# images: a READ-ONLY boot.vhd (S0 — /ScummVM/common: bank.ofsf plus whatever
# game data the user injects on-device; NO engine ELF) and a writable
# ScummVM.saves.vhd (S1 — one /ScummVM/<Instance> subfolder of preallocated
# config/save slots per instance ini).  Splitting them keeps saves on a separate,
# Downloader-reserved volume so an update can replace boot.vhd without touching
# the player's saves.  The engine ELFs ship as LOOSE files next to boot.vhd: the
# firmware F-loads the one named by the launched instance's ini at ioctl index 2
# into ELF staging and serves it as slot 3, so an engine update is a loose-file
# swap and boot.vhd never changes.  Also ships: a 4-line .mgl launcher per
# instance (S0 boot + S1 saves + F-load ELF index 2 + F-load ini index 1; ELF
# BEFORE ini — the ini F-load resets and triggers the app load), the loose
# per-instance inis, setup.sh (seeds the saves image + injects the user's game
# data) and external_files.csv (freeware data list).
# `make package TARGET=mister` builds and zips this bundle.
#
# GAME_INIS_SRC: canonical per-instance inis, shared with the Pocket asset tree
# GAME_INIS    : the subset that ships as MiSTer instances (staged copy — the
#                pocket-only exampleControlMap_* doc sample is not a game)
# GAME_STAGE   : games/OpenfpgaOS/-rooted staging that package.sh zips
# GAME_COMMON  : throwaway dir feeding mkgame the bank.ofsf (kept OUT of the
#                staging so it is never zipped)
# GAME_ELF     : the primary engine ELF (SCUMM); every engine ELF ships, and
#                each instance's ini names the one it F-loads
# BOOT_MB      : boot.vhd size.  ScummVM game data is far larger than a Doom
#                wad (talkie CD releases are hundreds of MB) and all instances
#                share ONE boot shell, so the shell is sized for a usable
#                injection budget instead of mkgame's 48 MB floor.  The image is
#                empty (sparse) — it costs SD space, not download size.
GAME          = ScummVM
GAME_INIS_SRC = $(DIST)/Assets/$(APP)/common
GAME_INIS     = $(ROOT)/build/mister/.$(APP)-inis
GAME_INI_SKIP = exampleControlMap_tentacle_os.ini
GAME_STAGE    = $(ROOT)/build/mister/$(APP)
GAME_COMMON   = $(ROOT)/build/mister/.$(APP)-common
GAME_ELF      = scummvm_lucasarts.elf
GAME_CSV      = $(DIST)/mister/external_files.csv
BOOT_MB      ?= 1024

game-dist: build
	@rm -rf "$(GAME_STAGE)" "$(GAME_COMMON)" "$(GAME_INIS)"
	@mkdir -p "$(GAME_STAGE)/$(GAME)/wads" "$(GAME_COMMON)" "$(GAME_INIS)"
	@cp "$(RUNTIME)/bank.ofsf" "$(GAME_COMMON)/bank.ofsf"
	@for f in $(GAME_INIS_SRC)/*.ini; do \
	    case "$$(basename $$f)" in $(GAME_INI_SKIP)) continue ;; esac; \
	    cp "$$f" "$(GAME_INIS)/"; \
	done
	@$(SDK_DIR)/platforms/mister/mkgame.sh --no-elf --saves-out "$(GAME_STAGE)/$(GAME)/$(GAME).saves.vhd" "$(GAME)" "$(GAME_INIS)" "$(GAME_COMMON)" "$(GAME_STAGE)/$(GAME)/boot.vhd" $(BOOT_MB)
	@$(SDK_DIR)/platforms/mister/mkmgl.sh "$(GAME)" "$(GAME_INIS)" "$(GAME_STAGE)"
	@cp $(ENGINE_ELFS) "$(GAME_STAGE)/$(GAME)/"
	@cp "$(GAME_INIS)"/*.ini "$(GAME_STAGE)/$(GAME)/"
	@sed 's|^GAME_DEFAULT=.*|GAME_DEFAULT="$(GAME)"|' "$(SDK_DIR)/platforms/mister/setup.sh" > "$(GAME_STAGE)/$(GAME)/setup.sh"
	@chmod +x "$(GAME_STAGE)/$(GAME)/setup.sh"
	@cp "$(GAME_CSV)" "$(GAME_STAGE)/$(GAME)/external_files.csv"
	@rm -rf "$(GAME_COMMON)" "$(GAME_INIS)"
	@echo "  game-dist staged -> $(GAME_STAGE)"

# UART push + console: debug the SCUMM (lucasarts) ELF by default.
debug: all
	@$(ROOT)/scripts/debug.sh "$(COMMON)/scummvm_$(OUT_$(firstword $(ENGINES))).elf"

clean:
	rm -rf $(BUILD) $(OUT)
	find . -name '*.o' -delete
