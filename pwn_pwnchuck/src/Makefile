# PwnChuck build entry points.
#
#   make dev                        debug build with symbols, no flag payload
#   make dev FLAG='srdnlen{...}'    debug build with a payload (local testing)
#   make dist                       stripped player build, no flag payload
#   make release FLAG='...'         stripped station build, embedded payload
#   make check                      host tests + artifact checks
#   make clean
#
# The Wii build itself is Makefile.wii (a devkitPro makefile); run everything
# through this wrapper so the flag payload is generated for the right target.
#
# Typical (devkitPro docker image):
#   docker run --rm -v "$PWD":/project -w /project devkitpro/devkitppc:latest make dist

DEVKITPRO ?= /opt/devkitpro
PATH := $(DEVKITPRO)/tools/bin:$(DEVKITPRO)/devkitPPC/bin:$(PATH)
export PATH

STRIP ?= powerpc-eabi-strip

DIST := dist
PLAYER := $(DIST)/pwnchuck-player
STATION := $(DIST)/pwnchuck-station
.PHONY: dev dist release check clean

# ---------------------------------------------------------------------------

dev:
	python3 tools/mkflag.py $(if $(FLAG),--flag '$(FLAG)',--none) --out-dir source
	$(MAKE) -f Makefile.wii PWNCHUCK_DEV=1 PWNCHUCK_EMU_CONTROLS=1 BUILD=build-dev
	python3 tools/mkflag.py --none --out-dir source
	@mkdir -p $(DIST)
	@cp pwnchuck.elf $(DIST)/pwnchuck-dev.elf
	@echo "dev build: $(DIST)/pwnchuck-dev.elf (symbols kept, dev cheats on, payload $(if $(FLAG),present,none))"

dist:
	python3 tools/mkflag.py --none --out-dir source
	$(MAKE) -f Makefile.wii PWNCHUCK_EMU_CONTROLS=1 BUILD=build-dist
	@mkdir -p $(DIST)
	cp pwnchuck.elf $(PLAYER).elf
	cp pwnchuck.dol $(PLAYER).dol
	#$(STRIP) -s $(PLAYER).elf
	@echo "player build: $(PLAYER).elf $(PLAYER).dol"

release:
	@test -n "$(FLAG)" || { echo "usage: make release FLAG='srdnlen{...}'"; exit 1; }
	python3 tools/mkflag.py --flag '$(FLAG)' --out-dir source
	$(MAKE) -f Makefile.wii BUILD=build-release
	@mkdir -p $(DIST)
	cp pwnchuck.elf $(STATION).elf
	cp pwnchuck.dol $(STATION).dol
	$(STRIP) -s $(STATION).elf
	python3 tools/mkflag.py --none --out-dir source
	@echo "station build: $(STATION).elf $(STATION).dol"

check:
	python3 tools/mkflag.py --selftest
	$(MAKE) -C tests clean all
	python3 tools/verify_builds.py $(PLAYER).elf $(STATION).elf $(if $(FLAG),--flag '$(FLAG)',)

clean:
	$(MAKE) -f Makefile.wii BUILD=build-dev clean
	$(MAKE) -f Makefile.wii BUILD=build-dist clean
	$(MAKE) -f Makefile.wii BUILD=build-release clean
	$(MAKE) -C tests clean
	rm -rf $(DIST)
