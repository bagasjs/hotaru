#include "hvm.h"
#include <stdio.h>

void make_module(void);

int main(void)
{
    printf("Creating the module\n");
    make_module();

    printf("Loading && Running the module\n");
    Arena a;
    HVM vm;
    HVM_Module mod;
    hvm_module_init(&mod);
    hvm_module_load_from_file(&mod, "build/sample.hvm", &a);
    hvm_module_dump(mod);

    hvm_init(&vm);
    hvm_exec_module(&vm, mod);

    hvm_module_deinit(&mod);
    return 0;
}

void make_module(void)
{
    HVM_Inst inst;
    HVM_Module mod;
    HVM vm;

    hvm_module_init(&mod);

    inst.type = HVM_INST_PUSH;
    inst.op.as_i64 = 0;
    hvm_module_append(&mod, inst);

    inst.type = HVM_INST_PUSH;
    inst.op.as_i64 = 1;
    hvm_module_append(&mod, inst);

    inst.type = HVM_INST_ADD;
    hvm_module_append(&mod, inst);

    inst.type = HVM_INST_COPY;
    inst.op.as_i64 = 0;
    hvm_module_append(&mod, inst);

    inst.type = HVM_INST_PUSH;
    inst.op.as_i64 = 100;
    hvm_module_append(&mod, inst);

    inst.type = HVM_INST_LT;
    hvm_module_append(&mod, inst);

    inst.type = HVM_INST_JN;
    inst.op.as_i64 = 1;
    hvm_module_append(&mod, inst);

    inst.type = HVM_INST_HALT;
    hvm_module_append(&mod, inst);

    hvm_module_dump(mod);
    hvm_exec_module(&vm, mod);
    hvm_module_save_to_file(mod, "build/sample.hvm");

    free(mod.items);
}

