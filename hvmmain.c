#include "hvm.h"
#include <stdio.h>

int main(int argc, const char **argv)
{
    if(argc < 2) {
        fprintf(stderr, "ERROR: Please provide a hvm bytecode file as an argument\n");
        fprintf(stderr, "USAGE: %s <program.hbc>\n", argv[0]);
        return -1;
    }

    Arena a = {0};
    HVM_Module mod = {0};
    if(!hvm_module_load_from_file(&mod, argv[1], &a)) {
        fprintf(stderr, "ERROR: Could not load file %s\n", argv[1]);
        fprintf(stderr, "USAGE: %s <program.hbc>\n", argv[0]);
        return -1;
    }

    HVM vm;
    HVM_Trap trap = hvm_exec_module(&vm, mod);
    hvm_module_deinit(&mod);
    arena_free(&a);
    return trap;
}
