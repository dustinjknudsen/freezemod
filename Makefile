CXX    := x86_64-w64-mingw32-g++
SRCDIR := src
OUTDIR := build

# Override at ship time: make install MODNAME=baseq2
MODNAME ?= freezemod

DEFINES := \
	-DKEX_Q2_GAME \
	-DKEX_Q2GAME_EXPORTS \
	-DKEX_Q2GAME_DYNAMIC \
	-DFMT_HEADER_ONLY \
	-DNO_FMT_SOURCE \
	-D_CRT_SECURE_NO_WARNINGS

CXXBASE := -std=c++17 \
	-Wall \
	-Wno-unused-parameter \
	-Wno-sign-compare \
	-Wno-missing-field-initializers \
	-I$(SRCDIR) \
	$(DEFINES)

BUILD ?= release

ifeq ($(BUILD),debug)
CXXFLAGS := $(CXXBASE) -g
OBJDIR   := $(OUTDIR)/obj-debug
TARGET   := $(OUTDIR)/game_x64d.dll
else
CXXFLAGS := $(CXXBASE) -O2 -DNDEBUG
OBJDIR   := $(OUTDIR)/obj-release
TARGET   := $(OUTDIR)/game_x64.dll
endif

LDFLAGS := -shared -static-libstdc++ -static-libgcc

PROTON_PREFIX  := /data/SteamLibrary/steamapps/compatdata/2320/pfx
SAVED_GAMES    := $(PROTON_PREFIX)/drive_c/users/steamuser/Saved Games
MODDIR_SAVES   := $(SAVED_GAMES)/Nightdive Studios/Quake II/$(MODNAME)
GAME_INSTALL   := /data/SteamLibrary/steamapps/common/Quake 2/rerelease
MODDIR_INSTALL := $(GAME_INSTALL)/$(MODNAME)

SRCS := \
	$(SRCDIR)/bots/bot_debug.cpp \
	$(SRCDIR)/bots/bot_exports.cpp \
	$(SRCDIR)/bots/bot_think.cpp \
	$(SRCDIR)/bots/bot_utils.cpp \
	$(SRCDIR)/cg_main.cpp \
	$(SRCDIR)/cg_screen.cpp \
	$(SRCDIR)/g_ai.cpp \
	$(SRCDIR)/g_ai_new.cpp \
	$(SRCDIR)/g_chase.cpp \
	$(SRCDIR)/g_cmds.cpp \
	$(SRCDIR)/g_combat.cpp \
	$(SRCDIR)/g_ctf.cpp \
	$(SRCDIR)/g_freeze.cpp \
	$(SRCDIR)/g_debug_log.cpp \
	$(SRCDIR)/g_func.cpp \
	$(SRCDIR)/g_items.cpp \
	$(SRCDIR)/g_main.cpp \
	$(SRCDIR)/g_menu.cpp \
	$(SRCDIR)/g_misc.cpp \
	$(SRCDIR)/g_monster.cpp \
	$(SRCDIR)/g_monster_spawn.cpp \
	$(SRCDIR)/g_phys.cpp \
	$(SRCDIR)/g_save.cpp \
	$(SRCDIR)/g_spawn.cpp \
	$(SRCDIR)/g_svcmds.cpp \
	$(SRCDIR)/g_target.cpp \
	$(SRCDIR)/g_trigger.cpp \
	$(SRCDIR)/g_turret.cpp \
	$(SRCDIR)/g_utils.cpp \
	$(SRCDIR)/g_weapon.cpp \
	$(SRCDIR)/json/jsoncpp.cpp \
	$(SRCDIR)/monsters/m_actor.cpp \
	$(SRCDIR)/monsters/m_arachnid.cpp \
	$(SRCDIR)/monsters/m_berserk.cpp \
	$(SRCDIR)/monsters/m_boss2.cpp \
	$(SRCDIR)/monsters/m_boss3.cpp \
	$(SRCDIR)/monsters/m_boss31.cpp \
	$(SRCDIR)/monsters/m_boss32.cpp \
	$(SRCDIR)/monsters/m_brain.cpp \
	$(SRCDIR)/monsters/m_carrier.cpp \
	$(SRCDIR)/monsters/m_chick.cpp \
	$(SRCDIR)/monsters/m_fixbot.cpp \
	$(SRCDIR)/monsters/m_flipper.cpp \
	$(SRCDIR)/monsters/m_float.cpp \
	$(SRCDIR)/monsters/m_flyer.cpp \
	$(SRCDIR)/monsters/m_gekk.cpp \
	$(SRCDIR)/monsters/m_gladiator.cpp \
	$(SRCDIR)/monsters/m_guardian.cpp \
	$(SRCDIR)/monsters/m_guncmdr.cpp \
	$(SRCDIR)/monsters/m_gunner.cpp \
	$(SRCDIR)/monsters/m_hover.cpp \
	$(SRCDIR)/monsters/m_infantry.cpp \
	$(SRCDIR)/monsters/m_insane.cpp \
	$(SRCDIR)/monsters/m_medic.cpp \
	$(SRCDIR)/monsters/m_move.cpp \
	$(SRCDIR)/monsters/m_mutant.cpp \
	$(SRCDIR)/monsters/m_parasite.cpp \
	$(SRCDIR)/monsters/m_shambler.cpp \
	$(SRCDIR)/monsters/m_soldier.cpp \
	$(SRCDIR)/monsters/m_stalker.cpp \
	$(SRCDIR)/monsters/m_supertank.cpp \
	$(SRCDIR)/monsters/m_tank.cpp \
	$(SRCDIR)/monsters/m_turret.cpp \
	$(SRCDIR)/monsters/m_widow.cpp \
	$(SRCDIR)/monsters/m_widow2.cpp \
	$(SRCDIR)/p_client.cpp \
	$(SRCDIR)/p_hud.cpp \
	$(SRCDIR)/p_menu.cpp \
	$(SRCDIR)/p_move.cpp \
	$(SRCDIR)/p_trail.cpp \
	$(SRCDIR)/p_view.cpp \
	$(SRCDIR)/p_weapon.cpp \
	$(SRCDIR)/q_std.cpp

OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all release debug clean distclean install

all: $(TARGET)

release:
	$(MAKE) BUILD=release

debug:
	$(MAKE) BUILD=debug

$(TARGET): $(OBJS) | $(OUTDIR)
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "==> $@"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUTDIR):
	@mkdir -p $@

install: $(OUTDIR)/game_x64.dll
	@mkdir -p "$(MODDIR_SAVES)"
	cp $(OUTDIR)/game_x64.dll "$(MODDIR_SAVES)/game_x64.dll"
	@mkdir -p "$(MODDIR_INSTALL)"
	@echo "==> DLL: $(MODDIR_SAVES)"
	@echo "==>      $(MODDIR_INSTALL)"

clean:
	rm -rf $(OUTDIR)

distclean: clean
	find $(SRCDIR) -name '*.o' -delete
	find $(SRCDIR) -name '*.d' -delete
