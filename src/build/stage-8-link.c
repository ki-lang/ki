
#include "../all.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Linker.h>
#include <llvm-c/Object.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/lto.h>

struct Target {
    LLVMTargetMachineRef target_machine;
    LLVMTargetDataRef target_data;
    char *llvm_triple;
    char *llvm_data_layout;
};

struct CompileData {
    Build *b;
    Array *ir_files;
    char *path_o;
    struct Target *target;
};

void llvm_init(Build *b, struct Target *t);
void *stage_8_compile_o(void *data_);
void stage_8_optimize(LLVMModuleRef mod);
void stage_8_link(Build *b, Array *o_files);

void stage_8(Build *b) {
    //

    bool compiled_any = false;
    Array *o_files = array_make(b->alc, 20);
    Array *threads = array_make(b->alc, 20);

#ifdef WIN32
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
#else
    struct timeval begin, end;
    gettimeofday(&begin, NULL);
#endif

    for (int i = 0; i < b->packages->length; i++) {
        Pkc *pkc = array_get_index(b->packages, i);
        for (int o = 0; o < pkc->namespaces->values->length; o++) {
            Nsc *nsc = array_get_index(pkc->namespaces->values, o);

            bool compile = false;
            Array *ir_files = array_make(b->alc, 20);

            for (int u = 0; u < nsc->fcs->length; u++) {
                Fc *fc = array_get_index(nsc->fcs, u);
                //
                if (fc->is_header) {
                    continue;
                }
                if (fc->ir_changed) {
                    fc_update_cache(fc);
                    compile = true;
                }

                //
                array_push(ir_files, fc->path_ir);
            }

            if (compile || !file_exists(nsc->path_o)) {
                compiled_any = true;
                if (b->verbose > 1) {
                    printf("⚙ Compile o file: %s\n", nsc->path_o);
                }

                struct CompileData *data = al(b->alc, sizeof(struct CompileData));
                data->b = b;
                data->ir_files = ir_files;
                data->path_o = nsc->path_o;
                data->target = al(b->alc, sizeof(struct Target));
                llvm_init(b, data->target);
                // stage_8_compile_o((void *)data);

#ifdef WIN32
                if (threads->length >= 16) {
                    // Wait for the first thread
                    void *thr = array_pop_first(threads);
                    WaitForSingleObject(thr, INFINITE);
                }

                void *thr = CreateThread(NULL, 0, (unsigned long (*)(void *))stage_8_compile_o, (void *)data, 0, NULL);
#else
                if (threads->length >= 16) {
                    // Wait for the first thread
                    pthread_t *thr = array_pop_first(threads);
                    pthread_join(*thr, NULL);
                }

                pthread_t *thr = al(b->alc, sizeof(pthread_t));
                pthread_create(thr, NULL, stage_8_compile_o, (void *)data);
#endif

                array_push(threads, thr);
            }

            array_push(o_files, nsc->path_o);
        }
    }

    for (int i = 0; i < threads->length; i++) {
#ifdef WIN32
        void *thr = array_get_index(threads, i);
        WaitForSingleObject(thr, INFINITE);
#else
        pthread_t *thr = array_get_index(threads, i);
        pthread_join(*thr, NULL);
#endif
    }

#ifdef WIN32
    QueryPerformanceCounter(&end);
    double time_llvm = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    QueryPerformanceCounter(&start);
#else
    gettimeofday(&end, NULL);
    double time_llvm = (double)(end.tv_usec - begin.tv_usec) / 1000000 + (double)(end.tv_sec - begin.tv_sec);
    gettimeofday(&begin, NULL);
#endif

    if (b->verbose > 0) {
        printf("⌚ LLVM build o: %.3fs\n", time_llvm);
    }

    if (!b->main_func) {
        die("❌ Missing 'main' function");
    }

    if (compiled_any || !file_exists(b->path_out)) {
        stage_8_link(b, o_files);
    }

#ifdef WIN32
    QueryPerformanceCounter(&end);
    double time_link = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
#else
    gettimeofday(&end, NULL);
    double time_link = (double)(end.tv_usec - begin.tv_usec) / 1000000 + (double)(end.tv_sec - begin.tv_sec);
#endif

    if (b->verbose > 0) {
        printf("⌚ Link: %.3fs\n", time_link);
    }
}

void *stage_8_compile_o(void *data_) {

    // LLVMStartMultithreaded();
    struct CompileData *data = (struct CompileData *)data_;
    Build *b = data->b;
    Array *ir_files = data->ir_files;
    char *path_o = data->path_o;
    struct Target *target = data->target;

    // struct timeval begin, end;
    // gettimeofday(&begin, NULL);
    //
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMContextSetOpaquePointers(ctx, true);

    LLVMModuleRef nsc_mod = LLVMModuleCreateWithNameInContext("ki_module", ctx);

    int ir_count = ir_files->length;
    int i = 0;
    while (i < ir_count) {
        char *ir_path = array_get_index(ir_files, i);
        i++;

        LLVMModuleRef mod;
        LLVMMemoryBufferRef buf = NULL;
        char *msg = NULL;

        LLVMBool check = LLVMCreateMemoryBufferWithContentsOfFile(ir_path, &buf, &msg);
        if (msg) {
            printf("LLVM load IR error: %s\n", msg);
            printf("Path: %s\n", ir_path);
            exit(1);
        }

        LLVMParseIRInContext(ctx, buf, &mod, &msg);
        if (msg) {
            printf("LLVM IR parse error: %s\n", msg);
            exit(1);
        }

        LLVMLinkModules2(nsc_mod, mod);
    }

    // Verify
    char *error = NULL;
    if (LLVMVerifyModule(nsc_mod, LLVMReturnStatusAction, &error) != 0) {
        char *ir_code = LLVMPrintModuleToString(nsc_mod);
        // printf("LLVM IR Code:\n%s\n", ir_code);
        printf("File: %s\n", path_o);
        printf("LLVM verify error: %s\n", error);
        exit(1);
    }

    // Compile
    error = NULL;
    LLVMSetTarget(nsc_mod, target->llvm_triple);
    LLVMSetDataLayout(nsc_mod, target->llvm_data_layout);

    if (b->optimize) {
        stage_8_optimize(nsc_mod);
    }

    if (LLVMTargetMachineEmitToFile(target->target_machine, nsc_mod, path_o, LLVMObjectFile, &error) != 0) {
        fprintf(stderr, "Failed to emit machine code!\n");
        fprintf(stderr, "Error: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    // printf("Object created: %s\n", outpath);

    LLVMDisposeMessage(error);
    LLVMDisposeModule(nsc_mod);

    // gettimeofday(&end, NULL);
    // double time_o = (double)(end.tv_usec - begin.tv_usec) / 1000000 + (double)(end.tv_sec - begin.tv_sec);
    // printf("⌚ %.3fs | %s\n", time_o, path_o);

    // LLVMStopMultithreaded();

    // return NULL;

#ifndef WIN32
    pthread_exit(NULL);
#endif
}

void stage_8_optimize(LLVMModuleRef mod) {

    LLVMPassManagerBuilderRef passBuilder = LLVMPassManagerBuilderCreate();

    // Note: O3 can produce slower code than O2 sometimes
    LLVMPassManagerBuilderSetOptLevel(passBuilder, 3);
    LLVMPassManagerBuilderSetSizeLevel(passBuilder, 0);
    LLVMPassManagerBuilderUseInlinerWithThreshold(passBuilder, 50);

    LLVMPassManagerRef func_passes = LLVMCreateFunctionPassManagerForModule(mod);
    LLVMPassManagerRef mod_passes = LLVMCreatePassManager();

    LLVMPassManagerBuilderPopulateFunctionPassManager(passBuilder, func_passes);
    LLVMPassManagerBuilderPopulateModulePassManager(passBuilder, mod_passes);

    // Other optimizations

    LLVMAddLoopDeletionPass(func_passes);
    LLVMAddLoopIdiomPass(func_passes);
    LLVMAddLoopRotatePass(func_passes);
    LLVMAddLoopRerollPass(func_passes);
    LLVMAddLoopUnrollPass(func_passes);
    LLVMAddLoopUnrollAndJamPass(func_passes);

    LLVMPassManagerBuilderDispose(passBuilder);
    LLVMInitializeFunctionPassManager(func_passes);

    for (LLVMValueRef func = LLVMGetFirstFunction(mod); func; func = LLVMGetNextFunction(func)) {
        LLVMRunFunctionPassManager(func_passes, func);
    }

    LLVMFinalizeFunctionPassManager(func_passes);
    LLVMRunPassManager(mod_passes, mod);

    LLVMDisposePassManager(func_passes);
    LLVMDisposePassManager(mod_passes);
}

void llvm_init(Build *b, struct Target *t) {
    //
    char *error = NULL;
    //
    LLVMInitializeNativeTarget();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();
    //
    // char *triple = LLVMGetDefaultTargetTriple();
    char *triple = NULL;
    char *cpu = "generic";
    if (b->target_os == os_linux) {
        if (b->target_arch == arch_x64) {
            triple = "x86_64-unknown-linux-gnu";
        } else if (b->target_arch == arch_arm64) {
            triple = "aarch64-unknown-linux-gnu";
        }
    } else if (b->target_os == os_macos) {
        if (b->target_arch == arch_x64) {
            triple = "x86_64-apple-darwin";
        } else if (b->target_arch == arch_arm64) {
            triple = "arm64-apple-darwin";
        }
    } else if (b->target_os == os_win) {
        if (b->target_arch == arch_x64) {
            triple = "x86_64-pc-windows-msvc";
        } else if (b->target_arch == arch_arm64) {
            triple = "aarch64-pc-windows-msvc";
        }
    }
    if (!triple) {
        die("❌ Could not figure out the LLVM triple");
    }

    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Failed to get target!\n");
        fprintf(stderr, "%s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(target, triple, cpu, NULL, LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);

    LLVMTargetDataRef datalayout = LLVMCreateTargetDataLayout(machine);
    char *datalayout_str = LLVMCopyStringRepOfTargetData(datalayout);

    t->target_machine = machine;
    t->target_data = LLVMCreateTargetDataLayout(t->target_machine);
    t->llvm_triple = triple;
    t->llvm_data_layout = datalayout_str;

    // printf("target: %s, [%s], %d, %d\n", LLVMGetTargetName(target), LLVMGetTargetDescription(target), LLVMTargetHasJIT(target), LLVMTargetHasTargetMachine(target));
    // printf("triple: %s\n", LLVMGetDefaultTargetTriple());
    // printf("features: %s\n", LLVMGetHostCPUFeatures());
    // printf("datalayout: %s\n", datalayout_str);
}

void stage_8_link_libs(Str *cmd, Build *b, int type) {
    //
    bool is_win = b->target_os == os_win;
    bool is_static = type == link_static;

    char *prefix = "";
    char *suffix = "";
    if (!is_win) {
        prefix = "-l";
        if (is_static) {
            prefix = "-l:lib";
            suffix = ".a";
        }
    } else {
        suffix = ".lib";
    }

    for (int i = 0; i < b->link_libs->values->length; i++) {
        char *name = array_get_index(b->link_libs->keys, i);
        Link *link = array_get_index(b->link_libs->values, i);
        if (link->type != type)
            continue;
        str_append_chars(cmd, prefix);
        str_append_chars(cmd, name);
        str_append_chars(cmd, suffix);
        str_append_chars(cmd, " ");
    }
}
void stage_8_link(Build *b, Array *o_files) {
    //
    Str *cmd = str_make(b->alc, 1000);
    bool is_linux = b->target_os == os_linux;
    bool is_macos = b->target_os == os_macos;
    bool is_win = b->target_os == os_win;

    bool is_x64 = strcmp(b->arch, "x64") == 0;
    bool is_arm64 = strcmp(b->arch, "arm64") == 0;

    bool host_os_is_target = b->host_os == b->target_os;
    bool host_arch_is_target = b->host_arch == b->target_arch;
    bool host_system_is_target = host_os_is_target && host_arch_is_target;

    char *linker = NULL;
    if (host_system_is_target) {
        if (is_linux) {
            linker = "ld";
        } else if (is_macos) {
            linker = "ld";
        } else if (is_win) {
            linker = "lld-link";
        }
    } else {
        if (is_linux) {
            linker = "ld.lld";
        } else if (is_macos) {
            linker = "ld64.lld";
        } else if (is_win) {
            linker = "lld-link";
        }
    }

    if (!linker) {
        die("❌ Could not figure out which linker to use for your host os / target os.");
    }

    //
    char *ki_lib_dir = b->pkc_ki->dir;

    str_append_chars(cmd, linker);
    str_append_chars(cmd, " ");
    if (is_win) {
        str_append_chars(cmd, "/out:");
    } else {
        str_append_chars(cmd, "-pie ");
        str_append_chars(cmd, "-o ");
    }
    str_append_chars(cmd, b->path_out);
    str_append_chars(cmd, " ");

    // Link dirs
    for (int i = 0; i < b->link_dirs->length; i++) {
        char *path = array_get_index(b->link_dirs, i);
        str_append_chars(cmd, is_win ? "/libpath:" : "-L");
        str_append_chars(cmd, path);
        str_append_chars(cmd, "/");
        str_append_chars(cmd, b->os);
        str_append_chars(cmd, "-");
        str_append_chars(cmd, b->arch);
        str_append_chars(cmd, " ");
    }

    // Details
    if (is_linux) {
        str_append_chars(cmd, "--sysroot=");
        str_append_chars(cmd, ki_lib_dir);
        str_append_chars(cmd, "/root ");

        if (is_x64) {
            str_append_chars(cmd, "-m elf_x86_64 ");
            str_append_chars(cmd, "--dynamic-linker /lib64/ld-linux-x86-64.so.2 ");
        } else if (is_arm64) {
            str_append_chars(cmd, "-m aarch64linux ");
            str_append_chars(cmd, "--dynamic-linker /lib/ld-linux-aarch64.so.1 ");
        }

        str_append_chars(cmd, "-l:Scrt1.o ");
        str_append_chars(cmd, "-l:crti.o ");

    } else if (is_macos) {
        str_append_chars(cmd, "-syslibroot ");
        str_append_chars(cmd, ki_lib_dir);
        str_append_chars(cmd, "/root ");

        // ppc, ppc64, i386, x86_64
        if (is_x64) {
            str_append_chars(cmd, "-arch x86_64 ");
        } else if (is_arm64) {
            str_append_chars(cmd, "-arch arm64 ");
        }
        str_append_chars(cmd, "-platform_version macos 10.13 11.1 ");
        // str_append_chars(cmd, "-sdk_version 11.1 ");
        // -macosx_version_min 11.1.0 -sdk_version 11.1.0
    } else if (is_win) {
        // /winsysroot:<value>
        str_append_chars(cmd, "/nodefaultlib ");
        // str_append_chars(cmd, "/force:unresolved ");
        if (is_x64) {
            str_append_chars(cmd, "/machine:x64 ");
        }
    }

    // Object files
    for (int i = 0; i < o_files->length; i++) {
        char *path = array_get_index(o_files, i);
        str_append_chars(cmd, path);
        str_append_chars(cmd, " ");
    }

    // Link libs
    stage_8_link_libs(cmd, b, link_dynamic);
    stage_8_link_libs(cmd, b, link_static);

    // End
    if (is_linux) {
        str_append_chars(cmd, "-l:crtn.o ");
    }

    // Run command
    char *cmd_str = str_to_chars(b->alc, cmd);
    if (b->verbose > 1) {
        printf("Link cmd: %s\n", cmd_str);
    }
    int res = system(cmd_str);
    if (res != 0) {
        die("❌ Failed to link");
    }
}