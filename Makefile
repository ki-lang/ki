
VERSION=dev
UNAME=$(shell uname)
UNAMEO=$(shell uname -o)

ifeq ($(UNAME), Darwin)
# From macos
CC=clang
LCC=clang
LLVM_CFG=llvm-config
else
# From linux
CC=clang-15
LCC=clang-15
LLVM_CFG=llvm-config-15
endif

CFLAGS:=-g -O2 -pthread `$(LLVM_CFG) --cflags`

LINK_DYNAMIC=`$(LLVM_CFG) --ldflags --system-libs --libs` -lcurl -lm -lz -lstdc++ -lpthread

LLVM_LIBS=-lLLVMOption -lLLVMObjCARCOpts -lLLVMMCJIT -lLLVMInterpreter -lLLVMExecutionEngine -lLLVMRuntimeDyld -lLLVMXCoreDisassembler -lLLVMXCoreCodeGen -lLLVMXCoreDesc -lLLVMXCoreInfo -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMWebAssemblyDisassembler -lLLVMWebAssemblyCodeGen -lLLVMWebAssemblyDesc -lLLVMWebAssemblyAsmParser -lLLVMWebAssemblyInfo -lLLVMWebAssemblyUtils -lLLVMSystemZDisassembler -lLLVMSystemZCodeGen -lLLVMSystemZAsmParser -lLLVMSystemZDesc -lLLVMSystemZInfo -lLLVMSparcDisassembler -lLLVMSparcCodeGen -lLLVMSparcAsmParser -lLLVMSparcDesc -lLLVMSparcInfo -lLLVMRISCVDisassembler -lLLVMRISCVCodeGen -lLLVMRISCVAsmParser -lLLVMRISCVDesc -lLLVMRISCVInfo -lLLVMPowerPCDisassembler -lLLVMPowerPCCodeGen -lLLVMPowerPCAsmParser -lLLVMPowerPCDesc -lLLVMPowerPCInfo -lLLVMNVPTXCodeGen -lLLVMNVPTXDesc -lLLVMNVPTXInfo -lLLVMMSP430Disassembler -lLLVMMSP430CodeGen -lLLVMMSP430AsmParser -lLLVMMSP430Desc -lLLVMMSP430Info -lLLVMMipsDisassembler -lLLVMMipsCodeGen -lLLVMMipsAsmParser -lLLVMMipsDesc -lLLVMMipsInfo -lLLVMLanaiDisassembler -lLLVMLanaiCodeGen -lLLVMLanaiAsmParser -lLLVMLanaiDesc -lLLVMLanaiInfo -lLLVMHexagonDisassembler -lLLVMHexagonCodeGen -lLLVMHexagonAsmParser -lLLVMHexagonDesc -lLLVMHexagonInfo -lLLVMBPFDisassembler -lLLVMBPFCodeGen -lLLVMBPFAsmParser -lLLVMBPFDesc -lLLVMBPFInfo -lLLVMAVRDisassembler -lLLVMAVRCodeGen -lLLVMAVRAsmParser -lLLVMAVRDesc -lLLVMAVRInfo -lLLVMARMDisassembler -lLLVMARMCodeGen -lLLVMARMAsmParser -lLLVMARMDesc -lLLVMARMUtils -lLLVMARMInfo -lLLVMAMDGPUDisassembler -lLLVMAMDGPUCodeGen -lLLVMMIRParser -lLLVMipo -lLLVMInstrumentation -lLLVMVectorize -lLLVMLinker -lLLVMIRReader -lLLVMAsmParser -lLLVMFrontendOpenMP -lLLVMAMDGPUAsmParser -lLLVMAMDGPUDesc -lLLVMAMDGPUUtils -lLLVMAMDGPUInfo -lLLVMAArch64Disassembler -lLLVMMCDisassembler -lLLVMAArch64CodeGen -lLLVMCFGuard -lLLVMGlobalISel -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMDebugInfoDWARF -lLLVMCodeGen -lLLVMTarget -lLLVMScalarOpts -lLLVMInstCombine -lLLVMAggressiveInstCombine -lLLVMTransformUtils -lLLVMBitWriter -lLLVMAnalysis -lLLVMProfileData -lLLVMObject -lLLVMTextAPI -lLLVMBitReader -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMAArch64AsmParser -lLLVMMCParser -lLLVMAArch64Desc -lLLVMMC -lLLVMDebugInfoCodeView -lLLVMDebugInfoMSF -lLLVMBinaryFormat -lLLVMAArch64Utils -lLLVMAArch64Info -lLLVMSupport -lLLVMDemangle -lLLVMPasses -lLLVMCoroutines -lLLVMVECodeGen -lLLVMVEAsmParser -lLLVMVEDesc -lLLVMVEDisassembler -lLLVMVEInfo

LLVM_LIBS_LINUX=-lLLVMM68kDisassembler -lLLVMM68kAsmParser -lLLVMM68kCodeGen -lLLVMM68kDesc -lLLVMM68kInfo -lLLVMXCoreDisassembler -lLLVMXCoreCodeGen -lLLVMXCoreDesc -lLLVMXCoreInfo -lLLVMX86TargetMCA -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMWebAssemblyDisassembler -lLLVMWebAssemblyAsmParser -lLLVMWebAssemblyCodeGen -lLLVMWebAssemblyDesc -lLLVMWebAssemblyUtils -lLLVMWebAssemblyInfo -lLLVMVEDisassembler -lLLVMVEAsmParser -lLLVMVECodeGen -lLLVMVEDesc -lLLVMVEInfo -lLLVMSystemZDisassembler -lLLVMSystemZAsmParser -lLLVMSystemZCodeGen -lLLVMSystemZDesc -lLLVMSystemZInfo -lLLVMSparcDisassembler -lLLVMSparcAsmParser -lLLVMSparcCodeGen -lLLVMSparcDesc -lLLVMSparcInfo -lLLVMRISCVDisassembler -lLLVMRISCVAsmParser -lLLVMRISCVCodeGen -lLLVMRISCVDesc -lLLVMRISCVInfo -lLLVMPowerPCDisassembler -lLLVMPowerPCAsmParser -lLLVMPowerPCCodeGen -lLLVMPowerPCDesc -lLLVMPowerPCInfo -lLLVMNVPTXCodeGen -lLLVMNVPTXDesc -lLLVMNVPTXInfo -lLLVMMSP430Disassembler -lLLVMMSP430AsmParser -lLLVMMSP430CodeGen -lLLVMMSP430Desc -lLLVMMSP430Info -lLLVMMipsDisassembler -lLLVMMipsAsmParser -lLLVMMipsCodeGen -lLLVMMipsDesc -lLLVMMipsInfo -lLLVMLanaiDisassembler -lLLVMLanaiCodeGen -lLLVMLanaiAsmParser -lLLVMLanaiDesc -lLLVMLanaiInfo -lLLVMHexagonDisassembler -lLLVMHexagonCodeGen -lLLVMHexagonAsmParser -lLLVMHexagonDesc -lLLVMHexagonInfo -lLLVMBPFDisassembler -lLLVMBPFAsmParser -lLLVMBPFCodeGen -lLLVMBPFDesc -lLLVMBPFInfo -lLLVMAVRDisassembler -lLLVMAVRAsmParser -lLLVMAVRCodeGen -lLLVMAVRDesc -lLLVMAVRInfo -lLLVMARMDisassembler -lLLVMARMAsmParser -lLLVMARMCodeGen -lLLVMARMDesc -lLLVMARMUtils -lLLVMARMInfo -lLLVMAMDGPUTargetMCA -lLLVMMCA -lLLVMAMDGPUDisassembler -lLLVMAMDGPUAsmParser -lLLVMAMDGPUCodeGen -lLLVMMIRParser -lLLVMAMDGPUDesc -lLLVMAMDGPUUtils -lLLVMAMDGPUInfo -lLLVMPasses -lLLVMObjCARCOpts -lLLVMCoroutines -lLLVMipo -lLLVMInstrumentation -lLLVMVectorize -lLLVMLinker -lLLVMIRReader -lLLVMAsmParser -lLLVMFrontendOpenMP -lLLVMAArch64Disassembler -lLLVMMCDisassembler -lLLVMAArch64AsmParser -lLLVMAArch64CodeGen -lLLVMCFGuard -lLLVMGlobalISel -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMCodeGen -lLLVMTarget -lLLVMScalarOpts -lLLVMInstCombine -lLLVMAggressiveInstCombine -lLLVMTransformUtils -lLLVMBitWriter -lLLVMAnalysis -lLLVMProfileData -lLLVMSymbolize -lLLVMDebugInfoPDB -lLLVMDebugInfoMSF -lLLVMDebugInfoDWARF -lLLVMObject -lLLVMTextAPI -lLLVMMCParser -lLLVMBitReader -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMAArch64Desc -lLLVMAArch64Utils -lLLVMAArch64Info -lLLVMMC -lLLVMDebugInfoCodeView -lLLVMBinaryFormat -lLLVMSupport -lLLVMDemangle

LINK_LIBS=$(LLVM_LIBS) -lm -lz -lpthread

SRC=$(wildcard src/*.c) $(wildcard src/libs/*.c) $(wildcard src/build/*.c) $(wildcard src/build/LLVM/*.c)
OBJECTS=$(patsubst %.c, debug/build/%.o, $(SRC))
OBJECTS_WIN_X64=$(patsubst %.c, debug/build-win-x64/%.o, $(SRC))
OBJECTS_LINUX_X64=$(patsubst %.c, debug/build-linux-x64/%.o, $(SRC))
OBJECTS_LINUX_ARM64=$(patsubst %.c, debug/build-linux-arm64/%.o, $(SRC))
OBJECTS_MACOS_X64=$(patsubst %.c, debug/build-macos-x64/%.o, $(SRC))
OBJECTS_MACOS_ARM64=$(patsubst %.c, debug/build-macos-arm64/%.o, $(SRC))
TARGET=ki

ki: $(OBJECTS)
	$(LCC) $(CFLAGS) -o $@ $(OBJECTS) $(LINK_DYNAMIC)

$(OBJECTS): debug/build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f ki $(OBJECTS) $(OBJECTS_WIN_X64) $(OBJECTS_LINUX_X64) $(OBJECTS_LINUX_ARM64) $(OBJECTS_MACOS_X64) $(OBJECTS_MACOS_ARM64)


##############
# DIST BULDS
##############

dist_setup:
	dist/toolchains.sh

$(OBJECTS_WIN_X64): debug/build-win-x64/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -g -O2 -arch=x86_64 --target=x86_64-pc-windows-msvc \
	-fms-compatibility-version=19 -fms-extensions -fdelayed-template-parsing -fexceptions -fno-threadsafe-statics \
	-mthread-model posix -Wno-msvc-not-found \
	-DWIN32 -D_WIN32 -D_MT -D_DLL \
	-Xclang -disable-llvm-verifier -Xclang '--dependent-lib=msvcrt' -Xclang '--dependent-lib=ucrt' -Xclang '--dependent-lib=oldnames' -Xclang '--dependent-lib=vcruntime' \
	-U__GNUC__ -U__gnu_linux__ -U__GNUC_MINOR__ -U__GNUC_PATCHLEVEL__ -U__GNUC_STDC_INLINE__ \
	--sysroot=$(CURDIR)/dist/toolchains/win-sdk -fuse-ld=lld \
	-I$(CURDIR)/dist/toolchains/win-sdk/Include/10.0.22621.0/ucrt \
	-I$(CURDIR)/dist/toolchains/win-sdk/Include/10.0.22621.0/um \
	-I$(CURDIR)/dist/toolchains/win-sdk/Include/10.0.22621.0/shared \
	-I$(CURDIR)/dist/toolchains/win-sdk/MSVC/14.36.32532/include \
	-I$(CURDIR)/dist/libraries/win-llvm-15-x64/include \
	-o $@ -c $<

dist_win_x64: $(OBJECTS_WIN_X64)
	mkdir -p dist/dists/win-x64
	rm -rf dist/dists/win-x64/*
	cp -r lib dist/dists/win-x64/
	cp -r src/scripts/install.bat dist/dists/win-x64/
	cp -r dist/libraries/win-llvm-15-x64/lld.exe dist/dists/win-x64/lld-link.exe
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/win-x64/install.bat

	$(LCC) -arch=x86_64 --target=x86_64-pc-windows-msvc -fuse-ld=lld \
	-L$(CURDIR)/dist/toolchains/win-sdk/Lib/10.0.22621.0/ucrt/x64 \
	-L$(CURDIR)/dist/toolchains/win-sdk/Lib/10.0.22621.0/um/x64 \
	-I$(CURDIR)/dist/toolchains/win-sdk/Include/10.0.22621.0/shared \
	-L$(CURDIR)/dist/toolchains/win-sdk/MSVC/14.36.32532/lib/x64 \
	-L$(CURDIR)/dist/libraries/win-llvm-15-x64/lib \
	-Wl,-machine:x64 \
	-o dist/dists/win-x64/ki.exe \
	$(OBJECTS_WIN_X64) \
	$(LLVM_LIBS) -nostdlib -lmsvcrt -Wno-msvc-not-found

	cd dist/dists/win-x64 && rm -f ../ki-$(VERSION)-win-x64.zip && zip -r ../ki-$(VERSION)-win-x64.zip ki.exe lib install.bat lld-link.exe

$(OBJECTS_LINUX_X64): debug/build-linux-x64/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -g -O2 -arch=x86_64 --target=x86_64-unknown-linux-gnu \
	--sysroot=$(CURDIR)/dist/toolchains/linux-x64/x86_64-buildroot-linux-gnu/sysroot \
	-I$(CURDIR)/dist/libraries/linux-llvm-15-x64/include \
	-o $@ -c $<

dist_linux_x64: $(OBJECTS_LINUX_X64)
	mkdir -p dist/dists/linux-x64
	rm -rf dist/dists/linux-x64/*
	cp -r lib dist/dists/linux-x64/
	cp -r src/scripts/install.sh dist/dists/linux-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/linux-x64/install.sh

	$(LCC) -arch=x86_64 --target=x86_64-unknown-linux-gnu -fuse-ld=lld \
	--sysroot=$(CURDIR)/dist/toolchains/linux-x64/x86_64-buildroot-linux-gnu/sysroot \
	-L$(CURDIR)/dist/toolchains/linux-x64/lib \
	-L$(CURDIR)/dist/libraries/linux-llvm-15-x64/lib \
	-o dist/dists/linux-x64/ki \
	$(OBJECTS_LINUX_X64) \
	$(LLVM_LIBS_LINUX) -lc -lstdc++ -lrt -ldl -lpthread -lm -lz -ltinfo -lxml2

	cd dist/dists/linux-x64 && rm -f ../ki-$(VERSION)-linux-x64.tar.gz && tar -czf ../ki-$(VERSION)-linux-x64.tar.gz ki lib install.sh


$(OBJECTS_LINUX_ARM64): debug/build-linux-arm64/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -g -O2 -arch=aarch64 --target=aarch64-unknown-linux-gnu \
	--sysroot=$(CURDIR)/dist/toolchains/linux-arm64/aarch64-buildroot-linux-gnu/sysroot \
	-I$(CURDIR)/dist/libraries/linux-llvm-15-arm64/include \
	-o $@ -c $<

dist_linux_arm64: $(OBJECTS_LINUX_ARM64)
	mkdir -p dist/dists/linux-arm64
	rm -rf dist/dists/linux-arm64/*
	cp -r lib dist/dists/linux-arm64/
	cp -r src/scripts/install.sh dist/dists/linux-arm64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/linux-arm64/install.sh

	$(LCC) -arch=aarch64 --target=aarch64-unknown-linux-gnu -fuse-ld=lld \
	--sysroot=$(CURDIR)/dist/toolchains/linux-arm64/aarch64-buildroot-linux-gnu/sysroot \
	-L$(CURDIR)/dist/libraries/linux-llvm-15-arm64/lib \
	-o dist/dists/linux-arm64/ki \
	$(OBJECTS_LINUX_ARM64) \
	$(LLVM_LIBS_LINUX) -lc -lstdc++ -lrt -ldl -lpthread -lm -lz -ltinfo -lxml2

	cd dist/dists/linux-arm64 && rm -f ../ki-$(VERSION)-linux-arm64.tar.gz && tar -czf ../ki-$(VERSION)-linux-arm64.tar.gz ki lib install.sh


$(OBJECTS_MACOS_X64): debug/build-macos-x64/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -g -O2 --target=x86_64-apple-darwin-macho \
	--sysroot=$(CURDIR)/dist/toolchains/macos-11-3 \
	-I$(CURDIR)/dist/libraries/macos-llvm-15-x64/include \
	-o $@ -c $<

dist_macos_x64: $(OBJECTS_MACOS_X64)
	mkdir -p dist/dists/macos-x64
	rm -rf dist/dists/macos-x64/*
	cp -r lib dist/dists/macos-x64/
	cp -r src/scripts/install.sh dist/dists/macos-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/macos-x64/install.sh

	export SDKROOT=$(CURDIR)/dist/toolchains/macos-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) -arch=x86_64 --target=x86_64-apple-darwin-macho -fuse-ld=lld \
	--sysroot=$(CURDIR)/dist/toolchains/macos-11-3 \
	-L$(CURDIR)/dist/libraries/macos-llvm-15-x64/lib \
	-o dist/dists/macos-x64/ki \
	-Wl,-platform_version,macos,11.6.0,11.3 \
	$(OBJECTS_MACOS_X64) \
	$(LLVM_LIBS) -lcurses -lc++ -lz

	cd dist/dists/macos-x64 && rm -f ../ki-$(VERSION)-macos-x64.tar.gz && tar -czf ../ki-$(VERSION)-macos-x64.tar.gz ki lib install.sh


$(OBJECTS_MACOS_ARM64): debug/build-macos-arm64/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -g -O2 --target=arm64-apple-darwin-macho \
	--sysroot=$(CURDIR)/dist/toolchains/macos-11-3 \
	-I$(CURDIR)/dist/libraries/macos-llvm-15-arm64/include \
	-o $@ -c $<

dist_macos_arm64: $(OBJECTS_MACOS_ARM64)
	mkdir -p dist/dists/macos-arm64
	rm -rf dist/dists/macos-arm64/*
	cp -r lib dist/dists/macos-arm64/
	cp -r src/scripts/install.sh dist/dists/macos-arm64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/macos-arm64/install.sh

	export SDKROOT=$(CURDIR)/dist/toolchains/macos-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) -arch=arm64 --target=arm64-apple-darwin-macho -fuse-ld=lld \
	--sysroot=$(CURDIR)/dist/toolchains/macos-11-3 \
	-L$(CURDIR)/dist/libraries/macos-llvm-15-arm64/lib \
	-o dist/dists/macos-arm64/ki \
	-Wl,-platform_version,macos,11.6.0,11.3 \
	$(OBJECTS_MACOS_ARM64) \
	$(LLVM_LIBS) -lcurses -lc++ -lz

	cd dist/dists/macos-arm64 && rm -f ../ki-$(VERSION)-macos-arm64.tar.gz && tar -czf ../ki-$(VERSION)-macos-arm64.tar.gz ki lib install.sh

dist_all: dist_win_x64 dist_linux_x64 dist_linux_arm64 dist_macos_x64 dist_macos_arm64

.PHONY: clean dist_setup dist_all dist_win_x64 dist_linux_x64 dist_linux_arm64 dist_macos_x64 dist_macos_arm64
