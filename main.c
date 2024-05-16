#include "hotaru.h"
#include "hvm.h"
#include "utils.h"
#include <stdio.h>

void usage(FILE *f, const char *program)
{
    fprintf(f, "USAGE: %s SUBCOMMAND <ARGS> [FLAGS]\n", program);
    fprintf(f, "Available subcommand:\n");
    fprintf(f, "    com  <file.ht> -o <output.hbc>\n");
    fprintf(f, "    run  <file.ht>\n");
    fprintf(f, "    dump <file.hbc>\n");
    fprintf(f, "    help\n");
}

typedef struct Args {
    int count;
    const char **items;
} Args;

const char *shift_args(Args *args, const char *on_empty_message)
{
    if(args->count <= 0) {
        fprintf(stderr, "ERROR: %s\n", on_empty_message);
        exit(EXIT_FAILURE);
        return NULL;
    }
    const char *result = args->items[0];
    args->items += 1;
    args->count -= 1;
    return result;
}

enum cli_mode {
    cli_mode_run,
    cli_mode_compile,
    cli_mode_dump,
};

int main(int argc, const char **argv)
{
    Args args = {argc, argv};
    const char *program_name = shift_args(&args, "Unreachable");
    StringView subcommand = sv_from_cstr(shift_args(&args, "Please provide a subcommand"));
    enum cli_mode mode = ut_false;

    if(sv_eq(subcommand, SV("help"))) {
        usage(stdout, program_name);
        return 0;
    } else if(sv_eq(subcommand, SV("com"))) {
        mode = cli_mode_compile;
    } else if(sv_eq(subcommand, SV("run"))) {
        mode = cli_mode_run;
    } else if(sv_eq(subcommand, SV("bcdump"))) {
        mode = cli_mode_dump;
    } else {
        fprintf(stderr, "ERROR: Invalid subcommand %s\n", subcommand.data);
        usage(stderr, program_name);
        return -1;
    }

    const char *source_file = shift_args(&args, "Provide the source file path");
    const char *output_file = "output.hbc";

    if(mode == cli_mode_compile && args.count > 0) {
        StringView flag = sv_from_cstr(shift_args(&args, "Unreachable"));
        if(!sv_eq(flag, SV("-o"))) {
            fprintf(stderr, "ERROR: Invalid flag %s\n", flag.data);
            usage(stderr, program_name);
            return -1;
        }
        output_file = shift_args(&args, "Expecting output file path");
    }

    Arena a = {0};
    hState state;
    hstate_init(&state);

    switch(mode) {
        case cli_mode_run:
            {
                char *source = load_file_text_with_arena(source_file, &a);
                if(!source) {
                    fprintf(stderr, "ERROR: Could not load file %s\n", argv[1]);
                    usage(stderr, argv[0]);
                    return -1;
                }
                hstate_exec_source(&state, source);
                hvm_dump(&state.vm);
            } break;
        case cli_mode_dump:
            {
                HVM_Module mod = {0};
                if(!hvm_module_load_from_file(&mod, source_file, &a)) {
                    fprintf(stderr, "ERROR: Could not load file %s\n", argv[1]);
                    fprintf(stderr, "USAGE: %s <program.hbc>\n", argv[0]);
                    return -1;
                }
                hvm_module_dump(mod);
                hvm_module_deinit(&mod);
            } break;
        case cli_mode_compile:
            {
                char *source = load_file_text_with_arena(source_file, &a);
                if(!source) {
                    fprintf(stderr, "ERROR: Could not load file %s\n", argv[1]);
                    usage(stderr, argv[0]);
                    return -1;
                }
                hstate_compile_source(&state, source);
                hvm_module_save_to_file(state.mod, output_file);
            } break;
    }

    arena_free(&a);
    hstate_deinit(&state);
    return 0;
}
