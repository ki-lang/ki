
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
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include <llvm-c/Transforms/Scalar.h>

void llvm_init(Build *b);
void stage_8_compile_o(Build *b, Array *ir_files, char *path_o);
void stage_8_link(Build *b, Array *o_files);

LLVMTargetMachineRef g_target_machine;
LLVMTargetDataRef g_target_data;
char *g_llvm_triple;
char *g_llvm_data_layout;

void stage_8(Build *b) {
    //
    llvm_init(b);

    Array *o_files = array_make(b->alc, 20);

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
                    compile = true;
                }

                //
                array_push(ir_files, fc->path_ir);
            }

            if (compile) {
                if (b->verbose > 1) {
                    printf("⚙ Compile o file: %s\n", nsc->path_o);
                }
                stage_8_compile_o(b, ir_files, nsc->path_o);
            }

            array_push(o_files, nsc->path_o);
        }
    }

    stage_8_link(b, o_files);
}

void stage_8_compile_o(Build *b, Array *ir_files, char *path_o) {
    //
    LLVMContextRef ctx = LLVMGetGlobalContext();
    LLVMModuleRef nsc_mod = LLVMModuleCreateWithName("ki_module");

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
            printf("LLVM Load IR error: %s\n", msg);
            printf("Path: %s\n", ir_path);
            exit(1);
        }

        LLVMParseIRInContext(ctx, buf, &mod, &msg);
        if (msg) {
            printf("LLVM Error: %s\n", msg);
            exit(1);
        }

        LLVMLinkModules2(nsc_mod, mod);
    }

    // Verify
    char *error = NULL;
    if (LLVMVerifyModule(nsc_mod, LLVMReturnStatusAction, &error) != 0) {
        char *ir_code = LLVMPrintModuleToString(nsc_mod);
        printf("LLVM IR Code:\n%s\n", ir_code);
        printf("File: %s\n", path_o);
        printf("ERROR: %s\n", error);
        exit(1);
    }

    // Compile
    error = NULL;
    LLVMSetTarget(nsc_mod, g_llvm_triple);
    LLVMSetDataLayout(nsc_mod, g_llvm_data_layout);

    if (b->optimize) {
        // printf("Run LLVM optimization:\n");

        LLVMPassManagerBuilderRef pass_builder = LLVMPassManagerBuilderCreate();
        LLVMPassManagerBuilderSetOptLevel(pass_builder, 3);
        LLVMPassManagerBuilderSetSizeLevel(pass_builder, 0);
        // LLVMPassManagerBuilderUseInlinerWithThreshold(pass_builder, 1000000000);

        LLVMPassManagerRef function_passes = LLVMCreateFunctionPassManagerForModule(nsc_mod);
        LLVMPassManagerRef module_passes = LLVMCreatePassManager();
        LLVMPassManagerBuilderPopulateFunctionPassManager(pass_builder, function_passes);
        LLVMPassManagerBuilderPopulateModulePassManager(pass_builder, module_passes);
        LLVMInitializeFunctionPassManager(function_passes);
        for (LLVMValueRef value = LLVMGetFirstFunction(nsc_mod); value; value = LLVMGetNextFunction(value)) {
            LLVMRunFunctionPassManager(function_passes, value);
        }

        LLVMFinalizeFunctionPassManager(function_passes);
        LLVMRunPassManager(module_passes, nsc_mod);

        LLVMPassManagerBuilderDispose(pass_builder);
        LLVMDisposePassManager(function_passes);
        LLVMDisposePassManager(module_passes);
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

    if (b->verbose > 0) {
        printf("Link cmd: %s\n", cmd_str);
    }

    system(cmd_str);
}