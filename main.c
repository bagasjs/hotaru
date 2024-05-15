#include "hotaru.h"
#include "hvm.h"
#include "utils.h"
#include <stdio.h>

void usage(FILE *f, const char *program)
{
    fprintf(f, "USAGE: %s SUBCOMMAND <ARGS> [FLAGS]\n", program);
    fprintf(f, "Available subcommand:\n");
    fprintf(f, "    com <source.ht> -o <output.hbc>\n");
    fprintf(f, "    run <source.ht>\n");
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

int main(int argc, const char **argv)
{
    Args args = {argc, argv};
    const char *program_name = shift_args(&args, "Unreachable");
    StringView subcommand = sv_from_cstr(shift_args(&args, "Please provide a subcommand"));
    ut_bool compilation_mode = ut_false;

    if(sv_eq(subcommand, SV("help"))) {
        usage(stdout, program_name);
        return 0;
    } else if(sv_eq(subcommand, SV("com"))) {
        compilation_mode = ut_true;
    } else if(sv_eq(subcommand, SV("run"))) {
        compilation_mode = ut_false;
    } else {
        fprintf(stderr, "ERROR: Invalid subcommand %s\n", subcommand.data);
        usage(stderr, program_name);
        return -1;
    }

    const char *source_file = shift_args(&args, "Provide the source file path");
    const char *output_file = "output.hbc";

    if(compilation_mode && args.count > 0) {
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

    char *source = load_file_text_with_arena(source_file, &a);
    if(!source) {
        fprintf(stderr, "ERROR: Could not load file %s\n", argv[1]);
        usage(stderr, argv[0]);
        return -1;
    }

    if(compilation_mode) {
        hstate_compile_source(&state, source);
        hvm_module_save_to_file(state.mod, output_file);
    } else {
        hstate_exec_source(&state, source);
        hvm_dump(&state.vm);
    }
    hstate_deinit(&state);
    arena_free(&a);
    return 0;
}
