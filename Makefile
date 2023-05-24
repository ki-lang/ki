
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
LINK_STATIC=`$(LLVM_CFG) --ldflags --system-libs --libs --link-static all-targets` `curl-config --static-libs` -lm -lz -lstdc++ -lpthread

LLVM_LIBS=-lLLVMOption -lLLVMObjCARCOpts -lLLVMMCJIT -lLLVMInterpreter -lLLVMExecutionEngine -lLLVMRuntimeDyld -lLLVMXCoreDisassembler -lLLVMXCoreCodeGen -lLLVMXCoreDesc -lLLVMXCoreInfo -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMWebAssemblyDisassembler -lLLVMWebAssemblyCodeGen -lLLVMWebAssemblyDesc -lLLVMWebAssemblyAsmParser -lLLVMWebAssemblyInfo -lLLVMWebAssemblyUtils -lLLVMSystemZDisassembler -lLLVMSystemZCodeGen -lLLVMSystemZAsmParser -lLLVMSystemZDesc -lLLVMSystemZInfo -lLLVMSparcDisassembler -lLLVMSparcCodeGen -lLLVMSparcAsmParser -lLLVMSparcDesc -lLLVMSparcInfo -lLLVMRISCVDisassembler -lLLVMRISCVCodeGen -lLLVMRISCVAsmParser -lLLVMRISCVDesc -lLLVMRISCVInfo -lLLVMPowerPCDisassembler -lLLVMPowerPCCodeGen -lLLVMPowerPCAsmParser -lLLVMPowerPCDesc -lLLVMPowerPCInfo -lLLVMNVPTXCodeGen -lLLVMNVPTXDesc -lLLVMNVPTXInfo -lLLVMMSP430Disassembler -lLLVMMSP430CodeGen -lLLVMMSP430AsmParser -lLLVMMSP430Desc -lLLVMMSP430Info -lLLVMMipsDisassembler -lLLVMMipsCodeGen -lLLVMMipsAsmParser -lLLVMMipsDesc -lLLVMMipsInfo -lLLVMLanaiDisassembler -lLLVMLanaiCodeGen -lLLVMLanaiAsmParser -lLLVMLanaiDesc -lLLVMLanaiInfo -lLLVMHexagonDisassembler -lLLVMHexagonCodeGen -lLLVMHexagonAsmParser -lLLVMHexagonDesc -lLLVMHexagonInfo -lLLVMBPFDisassembler -lLLVMBPFCodeGen -lLLVMBPFAsmParser -lLLVMBPFDesc -lLLVMBPFInfo -lLLVMAVRDisassembler -lLLVMAVRCodeGen -lLLVMAVRAsmParser -lLLVMAVRDesc -lLLVMAVRInfo -lLLVMARMDisassembler -lLLVMARMCodeGen -lLLVMARMAsmParser -lLLVMARMDesc -lLLVMARMUtils -lLLVMARMInfo -lLLVMAMDGPUDisassembler -lLLVMAMDGPUCodeGen -lLLVMMIRParser -lLLVMipo -lLLVMInstrumentation -lLLVMVectorize -lLLVMLinker -lLLVMIRReader -lLLVMAsmParser -lLLVMFrontendOpenMP -lLLVMAMDGPUAsmParser -lLLVMAMDGPUDesc -lLLVMAMDGPUUtils -lLLVMAMDGPUInfo -lLLVMAArch64Disassembler -lLLVMMCDisassembler -lLLVMAArch64CodeGen -lLLVMCFGuard -lLLVMGlobalISel -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMDebugInfoDWARF -lLLVMCodeGen -lLLVMTarget -lLLVMScalarOpts -lLLVMInstCombine -lLLVMAggressiveInstCombine -lLLVMTransformUtils -lLLVMBitWriter -lLLVMAnalysis -lLLVMProfileData -lLLVMObject -lLLVMTextAPI -lLLVMBitReader -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMAArch64AsmParser -lLLVMMCParser -lLLVMAArch64Desc -lLLVMMC -lLLVMDebugInfoCodeView -lLLVMDebugInfoMSF -lLLVMBinaryFormat -lLLVMAArch64Utils -lLLVMAArch64Info -lLLVMSupport -lLLVMDemangle -lLLVMPasses -lLLVMCoroutines -lLLVMVECodeGen -lLLVMVEAsmParser -lLLVMVEDesc -lLLVMVEDisassembler -lLLVMVEInfo

LINK_LIBS=$(LLVM_LIBS) -lm -lz -lpthread

SRC=$(wildcard src/*.c) $(wildcard src/libs/*.c) $(wildcard src/build/*.c) $(wildcard src/build/LLVM/*.c)
OBJECTS=$(patsubst %.c, debug/build/%.o, $(SRC))
OBJECTS_WIN_X64=$(patsubst %.c, debug/build-win-x64/%.o, $(SRC))
TARGET=ki

ki: $(OBJECTS)
	$(LCC) $(CFLAGS) -o $@ $(OBJECTS) $(LINK_DYNAMIC)

$(OBJECTS): debug/build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f ki $(OBJECTS)

# STATIC DIST BULDS

linux_dist_from_linux:
	mkdir -p dist/dists/linux-x64/lib
	rm -rf dist/dists/linux-x64/lib
	cp -r lib dist/dists/linux-x64/
	cp -r src/scripts/install.sh dist/dists/linux-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/linux-x64/install.sh
	$(LCC) $(CFLAGS) -o dist/dists/linux-x64/ki $(SRC) $(LINK_STATIC)
	cd dist/dists/linux-x64 && tar -czf ../ki-$(VERSION)-linux-x64.tar.gz ki lib install.sh

linux_arm_dist_from_linux:
	mkdir -p dist/dists/linux-arm64/lib
	rm -rf dist/dists/linux-arm64/lib
	cp -r lib dist/dists/linux-arm64/
	cp -r src/scripts/install.sh dist/dists/linux-arm64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/linux-arm64/install.sh
	aarch64-linux-gnu-gcc $(CFLAGS) -o dist/dists/linux-arm64/ki $(SRC) $(LINK_STATIC)
	cd dist/dists/linux-arm64 && tar -czf ../ki-$(VERSION)-linux-arm64.tar.gz ki lib install.sh

win_dist_from_linux:
	mkdir -p dist/dists/win-x64/lib
	rm -rf dist/dists/win-x64/lib
	cp -r lib dist/dists/win-x64/
	cp -r src/scripts/install.bat dist/dists/win-x64/
	cp -r dist/deps/lld.exe dist/dists/win-x64/lld-link.exe
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/win-x64/install.bat
	/mnt/c/msys64/mingw64/bin/clang.exe -g -O2 -pthread -static -o dist/dists/win-x64/ki \
	`/mnt/c/msys64/mingw64/bin/llvm-config.exe --cflags` \
	$(SRC) \
	`/mnt/c/msys64/mingw64/bin/llvm-config.exe --ldflags --system-libs --libs --link-static all-targets` \
 	-lcurl -lm -lz -lstdc++ -lpthread
	cd dist/dists/win-x64 && zip -r ../ki-$(VERSION)-win-x64.zip ki.exe lib install.bat lld-link.exe

macos_dist_from_linux:
	mkdir -p dist/dists/macos-x64/lib
	rm -rf dist/dists/macos-x64/lib
	cp -r lib dist/dists/macos-x64/
	cp -r src/scripts/install.sh dist/dists/macos-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/macos-x64/install.sh
	export SDKROOT=$(CURDIR)/dist/toolchains/macos-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) -g -O2 -pthread -arch=x86_64 --target=x86_64-apple-darwin-macho \
	--sysroot=$(CURDIR)/dist/toolchains/macos-11-3 -fuse-ld=lld \
	-I$(CURDIR)/dist/libraries/macos-llvm-15-x64/include -L$(CURDIR)/dist/libraries/macos-llvm-15-x64/lib \
	-o dist/dists/macos-x64/ki $(SRC) -Wl,-platform_version,macos,11.6.0,11.3 \
	$(LINK_LIBS) -lcurses -lc++
	cd dist/dists/macos-x64 && tar -czf ../ki-$(VERSION)-macos-x64.tar.gz ki lib install.sh

macos_arm_dist_from_linux: 
	mkdir -p dist/dists/macos-arm64/lib
	rm -rf dist/dists/macos-arm64/lib
	cp -r lib dist/dists/macos-arm64/
	cp -r src/scripts/install.sh dist/dists/macos-arm64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/macos-arm64/install.sh
	export SDKROOT=$(CURDIR)/dist/toolchains/macos-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) -g -O2 -pthread -arch arm64 --target=arm64-apple-darwin-macho \
	--sysroot=$(CURDIR)/dist/toolchains/macos-11-3 -fuse-ld=lld \
	-I$(CURDIR)/dist/libraries/macos-llvm-15-arm64/include -L$(CURDIR)/dist/libraries/macos-llvm-15-arm64/lib \
	-o dist/dists/macos-arm64/ki $(SRC) -Wl,-platform_version,macos,11.6.0,11.3 \
	$(LINK_LIBS) -lcurses -lc++
	cd dist/dists/macos-arm64 && tar -czf ../ki-$(VERSION)-macos-arm64.tar.gz ki lib install.sh

dists_from_linux: linux_dist_from_linux win_dist_from_linux macos_dist_from_linux macos_arm_dist_from_linux
#

dists:
	dist/toolchains.sh

$(OBJECTS_WIN_X64): debug/build-win-x64/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -g -O2 -pthread -arch=x86_64 --target=x86_64-pc-windows-msvc \
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

dist_win: $(OBJECTS_WIN_X64)
	mkdir -p dist/dists/win-x64/lib
	rm -rf dist/dists/win-x64/lib
	cp -r lib dist/dists/win-x64/
	cp -r src/scripts/install.bat dist/dists/win-x64/
	cp -r dist/deps/lld.exe dist/dists/win-x64/lld-link.exe
	sed -i 's/__VERSION__/$(VERSION)/' dist/dists/win-x64/install.bat

	$(LCC) -g -O2 -pthread -arch=x86_64 --target=x86_64-pc-windows-msvc -fuse-ld=lld \
	-I$(CURDIR)/dist/toolchains/win-sdk/Include/10.0.22621.0/ucrt -L$(CURDIR)/dist/toolchains/win-sdk/Lib/10.0.22621.0/ucrt/x64 \
	-I$(CURDIR)/dist/toolchains/win-sdk/Include/10.0.22621.0/um -L$(CURDIR)/dist/toolchains/win-sdk/Lib/10.0.22621.0/um/x64 \
	-I$(CURDIR)/dist/toolchains/win-sdk/Include/10.0.22621.0/shared \
	-I$(CURDIR)/dist/toolchains/win-sdk/MSVC/14.36.32532/include -L$(CURDIR)/dist/toolchains/win-sdk/MSVC/14.36.32532/lib/x64 \
	-I$(CURDIR)/dist/libraries/win-llvm-15-x64/include -L$(CURDIR)/dist/libraries/win-llvm-15-x64/lib \
	-Wl,-machine:x64 \
	-o dist/dists/win-x64/ki.exe \
	$(OBJECTS_WIN_X64) \
	$(LLVM_LIBS) -nostdlib -lmsvcrt -Wno-msvc-not-found  \

	cd dist/dists/win-x64 && zip -r ../ki-$(VERSION)-win-x64.zip ki.exe lib install.bat lld-link.exe


.PHONY: clean linux macos linux_dist
