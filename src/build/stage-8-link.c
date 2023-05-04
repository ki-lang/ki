
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

#include <threads.h>

void llvm_init(Build *b);
void *stage_8_compile_o(void *data_);
void stage_8_optimize(LLVMModuleRef mod);
void stage_8_link(Build *b, Array *o_files);

thread_local LLVMTargetMachineRef g_target_machine;
thread_local LLVMTargetDataRef g_target_data;
thread_local char *g_llvm_triple;
thread_local char *g_llvm_data_layout;

struct CompileData {
    Build *b;
    Array *ir_files;
    char *path_o;
};

void stage_8(Build *b) {
    //

    bool compiled_any = false;
    Array *o_files = array_make(b->alc, 20);
    Array *threads = array_make(b->alc, 20);

    struct timeval begin, end;
    gettimeofday(&begin, NULL);

    // LLVMStartMultithreaded();

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
                    fc_update_cahce(fc);
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

                pthread_t *thr = al(b->alc, sizeof(pthread_t));
                struct CompileData *data = al(b->alc, sizeof(struct CompileData));
                data->b = b;
                data->ir_files = ir_files;
                data->path_o = nsc->path_o;
                // stage_8_compile_o((void *)data);

                if (threads->length >= 20) {
                    // Wait for the first thread
                    pthread_t *thr = array_pop_first(threads);
                    pthread_join(*thr, NULL);
                }

                pthread_create(thr, NULL, stage_8_compile_o, (void *)data);

                array_push(threads, thr);
            }

            array_push(o_files, nsc->path_o);
        }
    }

    for (int i = 0; i < threads->length; i++) {
        pthread_t *thr = array_get_index(threads, i);
        pthread_join(*thr, NULL);
    }

    // LLVMStopMultithreaded();

    gettimeofday(&end, NULL);
    double time_llvm = (double)(end.tv_usec - begin.tv_usec) / 1000000 + (double)(end.tv_sec - begin.tv_sec);
    gettimeofday(&begin, NULL);

    if (b->verbose > 0) {
        printf("⌚ LLVM build o: %.3fs\n", time_llvm);
    }

    if (!b->main_func) {
        die("❌ Missing 'main' function");
    }

    if (compiled_any || !file_exists(b->path_out)) {
        stage_8_link(b, o_files);
    }

    gettimeofday(&end, NULL);
    double time_link = (double)(end.tv_usec - begin.tv_usec) / 1000000 + (double)(end.tv_sec - begin.tv_sec);

    if (b->verbose > 0) {
        printf("⌚ Link: %.3fs\n", time_link);
    }
}

void *stage_8_compile_o(void *data_) {

    struct CompileData *data = (struct CompileData *)data_;
    Build *b = data->b;
    Array *ir_files = data->ir_files;
    char *path_o = data->path_o;

    // struct timeval begin, end;
    // gettimeofday(&begin, NULL);

    llvm_init(b);

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
    LLVMSetTarget(nsc_mod, g_llvm_triple);
    LLVMSetDataLayout(nsc_mod, g_llvm_data_layout);

    if (b->optimize) {
        stage_8_optimize(nsc_mod);
    }

    if (LLVMTargetMachineEmitToFile(g_target_machine, nsc_mod, path_o, LLVMObjectFile, &error) != 0) {
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

    return NULL;
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

    // LLVMAddInstructionCombiningPass(func_passes);

    // LLVMAddBasicAliasAnalysisPass(func_passes);
    // LLVMAddBasicAliasAnalysisPass(mod_passes);
    // LLVMAddTypeBasedAliasAnalysisPass(func_passes);
    // LLVMAddScopedNoAliasAAPass(func_passes);
    // LLVMAddAggressiveDCEPass(func_passes);
    // LLVMAddBitTrackingDCEPass(func_passes);

    LLVMAddLoopDeletionPass(func_passes);
    LLVMAddLoopIdiomPass(func_passes);
    LLVMAddLoopRotatePass(func_passes);
    LLVMAddLoopRerollPass(func_passes);
    LLVMAddLoopUnrollPass(func_passes);
    LLVMAddLoopUnrollAndJamPass(func_passes);
    // LLVMAddLoopUnswitchPass(func_passes);

    // LLVMAddScalarReplAggregatesPass(func_passes);
    // LLVMAddScalarReplAggregatesPassSSA(func_passes);
    // LLVMAddScalarReplAggregatesPassWithThreshold(func_passes, 10);
    // LLVMAddSimplifyLibCallsPass(func_passes);

    LLVMAddDemoteMemoryToRegisterPass(func_passes);
    // LLVMAddVerifierPass(func_passes);
    // LLVMAddCorrelatedValuePropagationPass(func_passes);

    // LLVMAddEarlyCSEPass(func_passes);
    // LLVMAddEarlyCSEMemSSAPass(func_passes);
    // LLVMAddLowerExpectIntrinsicPass(func_passes);

    // LLVMAddGlobalDCEPass(mod_passes);
    // LLVMAddGlobalOptimizerPass(mod_passes);
    // LLVMAddPruneEHPass(mod_passes);
    // LLVMAddIPSCCPPass(mod_passes);

    // LLVMAddArgumentPromotionPass(mod_passes);
    // LLVMAddConstantMergePass(mod_passes);
    // // LLVMAddInternalizePass(mod_passes, 10);
    // LLVMAddStripDeadPrototypesPass(mod_passes);
    // LLVMAddStripSymbolsPass(mod_passes);

    // LLVMAddCalledValuePropagationPass(mod_passes);
    // LLVMAddDeadArgEliminationPass(mod_passes);
    // // LLVMAddFunctionAttrsPass(mod_passes);
    // LLVMAddFunctionInliningPass(mod_passes);
    // LLVMAddAlwaysInlinerPass(mod_passes);

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

void llvm_init(Build *b) {
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
    char *triple = LLVMGetDefaultTargetTriple();
    char *cpu = "generic";

    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Failed to get target!\n");
        fprintf(stderr, "%s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(target, triple, cpu, LLVMGetHostCPUFeatures(), LLVMCodeGenLevelDefault, LLVMRelocPIC, LLVMCodeModelDefault);
    LLVMTargetDataRef datalayout = LLVMCreateTargetDataLayout(machine);
    char *datalayout_str = LLVMCopyStringRepOfTargetData(datalayout);

    g_target_machine = machine;
    g_target_data = LLVMCreateTargetDataLayout(g_target_machine);
    g_llvm_triple = triple;
    g_llvm_data_layout = datalayout_str;

    // printf("target: %s, [%s], %d, %d\n", LLVMGetTargetName(target), LLVMGetTargetDescription(target), LLVMTargetHasJIT(target), LLVMTargetHasTargetMachine(target));
    // printf("triple: %s\n", LLVMGetDefaultTargetTriple());
    // printf("features: %s\n", LLVMGetHostCPUFeatures());
    // printf("datalayout: %s\n", datalayout_str);
}

void stage_8_link(Build *b, Array *o_files) {
    //
    Str *cmd = str_make(b->alc, 1000);

    str_append_chars(cmd, "gcc -o ");
    str_append_chars(cmd, b->path_out);
    str_append_chars(cmd, " ");

    for (int i = 0; i < o_files->length; i++) {
        char *path = array_get_index(o_files, i);
        str_append_chars(cmd, path);
        str_append_chars(cmd, " ");
    }

    for (int i = 0; i < b->link_dirs->length; i++) {
        char *path = array_get_index(b->link_dirs, i);
        str_append_chars(cmd, "-L");
        str_append_chars(cmd, path);
        str_append_chars(cmd, "/");
        str_append_chars(cmd, b->os);
        str_append_chars(cmd, "-");
        str_append_chars(cmd, b->arch);
        str_append_chars(cmd, " ");
    }

    for (int i = 0; i < b->link_libs->length; i++) {
        char *name = array_get_index(b->link_libs, i);
        str_append_chars(cmd, "-l");
        str_append_chars(cmd, name);
        str_append_chars(cmd, " ");
    }

    // if (b->ldflags) {
    //     str_append_chars(cmd, b->ldflags);
    //     str_append_chars(cmd, " ");
    // }

    char *cmd_str = str_to_chars(b->alc, cmd);

    if (b->verbose > 1) {
        printf("Link cmd: %s\n", cmd_str);
    }

    int res = system(cmd_str);
    if (res != 0) {
        die("❌ Failed to link");
    }
}