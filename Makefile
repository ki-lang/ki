
VERSION=dev
UNAME=$(shell uname)

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

LINK_DYNAMIC=`$(LLVM_CFG) --ldflags --system-libs --libs` -lcurl -lm -lz -lstdc++ -ldl -lpthread
LINK_STATIC=`$(LLVM_CFG) --ldflags --system-libs --libs --link-static all-targets` `curl-config --static-libs` -lm -lz -lstdc++ -ldl -lpthread

LINK_LIBS=-lLLVMOption -lLLVMObjCARCOpts -lLLVMMCJIT -lLLVMInterpreter -lLLVMExecutionEngine -lLLVMRuntimeDyld -lLLVMXCoreDisassembler -lLLVMXCoreCodeGen -lLLVMXCoreDesc -lLLVMXCoreInfo -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMWebAssemblyDisassembler -lLLVMWebAssemblyCodeGen -lLLVMWebAssemblyDesc -lLLVMWebAssemblyAsmParser -lLLVMWebAssemblyInfo -lLLVMWebAssemblyUtils -lLLVMSystemZDisassembler -lLLVMSystemZCodeGen -lLLVMSystemZAsmParser -lLLVMSystemZDesc -lLLVMSystemZInfo -lLLVMSparcDisassembler -lLLVMSparcCodeGen -lLLVMSparcAsmParser -lLLVMSparcDesc -lLLVMSparcInfo -lLLVMRISCVDisassembler -lLLVMRISCVCodeGen -lLLVMRISCVAsmParser -lLLVMRISCVDesc -lLLVMRISCVInfo -lLLVMPowerPCDisassembler -lLLVMPowerPCCodeGen -lLLVMPowerPCAsmParser -lLLVMPowerPCDesc -lLLVMPowerPCInfo -lLLVMNVPTXCodeGen -lLLVMNVPTXDesc -lLLVMNVPTXInfo -lLLVMMSP430Disassembler -lLLVMMSP430CodeGen -lLLVMMSP430AsmParser -lLLVMMSP430Desc -lLLVMMSP430Info -lLLVMMipsDisassembler -lLLVMMipsCodeGen -lLLVMMipsAsmParser -lLLVMMipsDesc -lLLVMMipsInfo -lLLVMLanaiDisassembler -lLLVMLanaiCodeGen -lLLVMLanaiAsmParser -lLLVMLanaiDesc -lLLVMLanaiInfo -lLLVMHexagonDisassembler -lLLVMHexagonCodeGen -lLLVMHexagonAsmParser -lLLVMHexagonDesc -lLLVMHexagonInfo -lLLVMBPFDisassembler -lLLVMBPFCodeGen -lLLVMBPFAsmParser -lLLVMBPFDesc -lLLVMBPFInfo -lLLVMAVRDisassembler -lLLVMAVRCodeGen -lLLVMAVRAsmParser -lLLVMAVRDesc -lLLVMAVRInfo -lLLVMARMDisassembler -lLLVMARMCodeGen -lLLVMARMAsmParser -lLLVMARMDesc -lLLVMARMUtils -lLLVMARMInfo -lLLVMAMDGPUDisassembler -lLLVMAMDGPUCodeGen -lLLVMMIRParser -lLLVMipo -lLLVMInstrumentation -lLLVMVectorize -lLLVMLinker -lLLVMIRReader -lLLVMAsmParser -lLLVMFrontendOpenMP -lLLVMAMDGPUAsmParser -lLLVMAMDGPUDesc -lLLVMAMDGPUUtils -lLLVMAMDGPUInfo -lLLVMAArch64Disassembler -lLLVMMCDisassembler -lLLVMAArch64CodeGen -lLLVMCFGuard -lLLVMGlobalISel -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMDebugInfoDWARF -lLLVMCodeGen -lLLVMTarget -lLLVMScalarOpts -lLLVMInstCombine -lLLVMAggressiveInstCombine -lLLVMTransformUtils -lLLVMBitWriter -lLLVMAnalysis -lLLVMProfileData -lLLVMObject -lLLVMTextAPI -lLLVMBitReader -lLLVMCore -lLLVMRemarks -lLLVMBitstreamReader -lLLVMAArch64AsmParser -lLLVMMCParser -lLLVMAArch64Desc -lLLVMMC -lLLVMDebugInfoCodeView -lLLVMDebugInfoMSF -lLLVMBinaryFormat -lLLVMAArch64Utils -lLLVMAArch64Info -lLLVMSupport -lLLVMDemangle -lLLVMPasses -lLLVMCoroutines -lLLVMVECodeGen -lLLVMVEAsmParser -lLLVMVEDesc -lLLVMVEDisassembler -lLLVMVEInfo -lcurl -lm -lz -lpthread

SRC=$(wildcard src/*.c) $(wildcard src/libs/*.c) $(wildcard src/build/*.c) $(wildcard src/build/LLVM/*.c)
OBJECTS=$(patsubst %.c, debug/build/%.o, $(SRC))
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
	mkdir -p dist/linux-x64/lib
	rm -rf dist/linux-x64/lib
	cp -r lib dist/linux-x64/
	cp -r src/scripts/install.sh dist/linux-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/linux-x64/install.sh
	$(LCC) $(CFLAGS) -o dist/linux-x64/ki $(SRC) $(LINK_STATIC)
	cd dist/linux-x64 && tar -czf ../ki-$(VERSION)-linux-x64.tar.gz ki lib install.sh

linux_arm_dist_from_linux:
	mkdir -p dist/linux-arm64/lib
	rm -rf dist/linux-arm64/lib
	cp -r lib dist/linux-arm64/
	cp -r src/scripts/install.sh dist/linux-arm64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/linux-arm64/install.sh
	aarch64-linux-gnu-gcc $(CFLAGS) -o dist/linux-arm64/ki $(SRC) $(LINK_STATIC)
	cd dist/linux-arm64 && tar -czf ../ki-$(VERSION)-linux-arm64.tar.gz ki lib install.sh

win_dist_from_linux:
	mkdir -p dist/win-x64/lib
	rm -rf dist/win-x64/lib
	cp -r lib dist/win-x64/
	cp -r src/scripts/install.sh dist/win-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/win-x64/install.sh
	x86_64-w64-mingw32-gcc $(CFLAGS) -o dist/win-x64/ki $(SRC) $(LINK_LIBS) \
	-I$(CURDIR)/dist/curl/include -L$(CURDIR)/dist/curl/lib 
	#x86_64-w64-mingw32-gcc -g -O2 -pthread -o dist/win-x64/ki $(SRC) \
	#-I$(CURDIR)/dist/win-llvm-15-x64/include -L$(CURDIR)/dist/win-llvm-15-x64/lib \
	#-l:LLVM-C.lib -lcurl -lm -lz -lstdc++ -lpthread
	cd dist/win-x64 && tar -czf ../ki-$(VERSION)-win-x64.tar.gz ki lib install.sh

macos_dist_from_linux:
	mkdir -p dist/macos-x64/lib
	rm -rf dist/macos-x64/lib
	cp -r lib dist/macos-x64/
	cp -r src/scripts/install.sh dist/macos-x64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/macos-x64/install.sh
	export SDKROOT=$(CURDIR)/dist/macos-sdk-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) -g -O2 -pthread -arch=x86_64 --target=x86_64-apple-darwin-macho \
	--sysroot=$(CURDIR)/dist/macos-sdk-11-3 -fuse-ld=lld \
	-I$(CURDIR)/dist/macos-llvm-15-x64/include -L$(CURDIR)/dist/macos-llvm-15-x64/lib \
	-o dist/macos-x64/ki $(SRC) -Wl,-platform_version,macos,11.6.0,11.3 \
	$(LINK_LIBS) -lcurses -lc++
	cd dist/macos-x64 && tar -czf ../ki-$(VERSION)-macos-x64.tar.gz ki lib install.sh

macos_arm_dist_from_linux: 
	mkdir -p dist/macos-arm64/lib
	rm -rf dist/macos-arm64/lib
	cp -r lib dist/macos-arm64/
	cp -r src/scripts/install.sh dist/macos-arm64/
	sed -i 's/__VERSION__/$(VERSION)/' dist/macos-arm64/install.sh
	export SDKROOT=$(CURDIR)/dist/macos-sdk-11-3 && \
	export MACOSX_DEPLOYMENT_TARGET=11.6 && \
	$(LCC) -g -O2 -pthread -arch arm64 --target=arm64-apple-darwin-macho \
	--sysroot=$(CURDIR)/dist/macos-sdk-11-3 -fuse-ld=lld -I$(CURDIR)/dist/macos-llvm-15-arm64/include -L$(CURDIR)/dist/macos-llvm-15-arm64/lib \
	-o dist/macos-arm64/ki $(SRC) -Wl,-platform_version,macos,11.6.0,11.3 \
	$(LINK_LIBS) -lcurses -lc++
	cd dist/macos-arm64 && tar -czf ../ki-$(VERSION)-macos-arm64.tar.gz ki lib install.sh

dists_from_linux: linux_dist_from_linux macos_dist_from_linux macos_arm_dist_from_linux
#

.PHONY: clean linux macos linux_dist
