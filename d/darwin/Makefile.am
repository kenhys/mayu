CPPFLAGS_COMMON = \
		-F/System/Library/Frameworks/ \
		-I/System/Library/Frameworks/Kernel.framework/PrivateHeaders \
		-I/System/Library/Frameworks/Kernel.framework/Versions/A/Headers/ \
		-I/System/Library/Frameworks/Kernel.framework/Versions/A/Headers/IOKit/hidsystem/ \
		-I/System/Library/Frameworks/Kernel.framework/Versions/A/Headers/IOKit/hid/ \
		-I/System/Library/Frameworks/Kernel.framework/Versions/A/Headers/IOKit/hidevent/ \
		-I../..

if OS_TIGER
CPPFLAGS_COMMON += -I10.4 
else
if OS_LEOPARD
CPPFLAGS_COMMON += -I10.5
else
if OS_SNOW_LEOPARD
CPPFLAGS_COMMON += -I10.6
endif
endif
endif

# PPC support
if OS_SNOW_LEOPARD
MAYU_PPC =
else
MAYU_PPC = Mayu_ppc
endif

# PS2 support
if SUPPORT_PS2
CPPFLAGS_COMMON += -DPS2
INFO_PLIST = Info.plist.ps2
else
INFO_PLIST = Info.plist
endif

INCLUDE_FILES = \
		wintypes.h \
		common.h \
		PS2KeyboardHook.h \
		USBKeyboardHook.h \
		MayuDriver.h \
		device.h \
		key_queue.h \
		KeyboardMapperDelegater.h \
		ApplePS2Controller.h \
		ApplePS2Device.h \
		10.4/IOHIDConsumer.h \
		10.4/IOHIDKeyboard.h \
		10.5/IOHIDConsumer.h \
		10.5/IOHIDKeyboard.h \
		10.6/IOHIDConsumer.h \
		10.6/IOHIDKeyboard.h

CXXFLAGS_COMMON = \
		-pipe \
		-Wno-trigraphs \
		-fasm-blocks \
		-Os \
		-Wreturn-type \
		-Wunused-variable \
		-Wshadow \
		-fmessage-length=0 \
		-fapple-kext \
		-fno-common \
		-fno-builtin \
		-finline \
		-fno-keep-inline-functions \
		-force_cpusubtype_ALL \
		-fno-exceptions \
		-msoft-float \
		-static \
		-fno-rtti \
		-fcheck-new \
		-DKERNEL \
		-DKERNEL_PRIVATE \
		-DDRIVER_PRIVATE \
		-DAPPLE \
		-DNeXT \
		-no-cpp-precomp

CFLAGS_I386 = -arch i386
CFLAGS_X86_64 = -arch x86_64
CFLAGS_PPC = -arch ppc -mtune=G4 -mlong-branch

CFLAGS_KMOD = \
		-pipe \
		-Wno-trigraphs \
		-fasm-blocks \
		-Os \
		-Wreturn-type \
		-Wunused-variable \
		-Wshadow \
		-fmessage-length=0 \
		-fno-common \
		-fno-builtin \
		-finline \
		-fno-keep-inline-functions \
		-force_cpusubtype_ALL \
		-fno-exceptions \
		-msoft-float \
		-static \
		-DKERNEL \
		-DKERNEL_PRIVATE \
		-DDRIVER_PRIVATE \
		-DAPPLE \
		-DNeXT \
		-no-cpp-precomp

LINKFLAGS_COMMON = \
		-static \
		-nostdlib \
		-r \
		-Wl,-Y,1455

LINKFLAGS_I386 = -arch i386
LINKFLAGS_X86_64 = -arch x86_64
LINKFLAGS_PPC = -arch ppc

LINKLIBS = -lkmodc++ -lkmod -lcc_kext -lcpp_kext

SRC  = MayuDriver.cpp KeyboardMapperDelegater.cpp device.cpp key_queue.cpp MayuDriver_info.c

bin_PROGRAMS = Mayu.kext
noinst_PROGRAMS = Mayu_i386 Mayu_x86_64 $(MAYU_PPC) Mayu
Mayu_i386_SOURCES = $(SRC)
Mayu_x86_64_SOURCES = $(SRC)
Mayu_ppc_SOURCES = $(SRC)

Mayu_i386_CPPFLAGS = $(CPPFLAGS_COMMON)
Mayu_i386_CXXFLAGS = $(CXXFLAGS_COMMON) $(CFLAGS_I386)
Mayu_i386_CFLAGS = $(CFLAGS_KMOD) $(CFLAGS_I386)
Mayu_i386_LDFLAGS = $(LINKFLAGS_COMMON) $(LINKFLAGS_I386)
Mayu_i386_LDADD = $(LINKLIBS)

Mayu_x86_64_CPPFLAGS = $(CPPFLAGS_COMMON)
Mayu_x86_64_CXXFLAGS = $(CXXFLAGS_COMMON) $(CFLAGS_X86_64)
Mayu_x86_64_CFLAGS = $(CFLAGS_KMOD) $(CFLAGS_X86_64)
Mayu_x86_64_LDFLAGS = $(LINKFLAGS_COMMON) $(LINKFLAGS_X86_64)
Mayu_x86_64_LDADD = $(LINKLIBS)

Mayu_ppc_CPPFLAGS = $(CPPFLAGS_COMMON)
Mayu_ppc_CXXFLAGS = $(CXXFLAGS_COMMON) $(CFLAGS_PPC)
Mayu_ppc_CFLAGS = $(CFLAGS_KMOD) $(CFLAGS_PPC)
Mayu_ppc_LDFLAGS = $(LINKFLAGS_COMMON) $(LINKFLAGS_PPC)
Mayu_ppc_LDADD = $(LINKLIBS)

Mayu_SOURCES = $(INCLUDE_FILES)
Mayu_kext_SOURCES =

dist_noinst_DATA = Info.plist English.lproj/InfoPlist.strings

DEBUG_CXXFLAGS = "-g -D_DEBUG -DDEBUG"

Mayu : Mayu_i386 Mayu_x86_64 $(MAYU_PPC)
	lipo -create Mayu_i386 Mayu_x86_64 $(MAYU_PPC) -output Mayu

debug :
	$(MAKE) CXXFLAGS=$(DEBUG_CXXFLAGS)

Mayu.kext : Mayu $(INFO_PLIST) English.lproj
	mkdir -p Mayu.kext/Contents/MacOS
	mkdir -p Mayu.kext/Contents/Resorces
	cp $(INFO_PLIST) Mayu.kext/Contents/Info.plist
	cp -R English.lproj Mayu.kext/Contents/Resorces
	cp Mayu Mayu.kext/Contents/MacOS

install-exec-local :
	cp -r Mayu.kext /System/Library/Extensions
	kextload /System/Library/Extensions/Mayu.kext
	-rm -r /System/Library/Extensions.kextcache /System/Library/Extensions.mkext /System/Library.kextcache
	-kextcache -k /System/Library/

uninstall-local :
	-kextunload /System/Library/Extensions/Mayu.kext
	rm -r /System/Library/Extensions/Mayu.kext
	-rm -r /System/Library/Extensions.kextcache /System/Library/Extensions.mkext
	-kextcache -k /System/Library/
