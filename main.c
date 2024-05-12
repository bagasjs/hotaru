#include "hotaru.h"
#include "hvm.h"

int main(void)
{
    hState state;
    hstate_init(&state);

    hstate_exec_source(&state, "var hello = 35;");
    hvm_dump(&state.vm);

    hstate_exec_source(&state, "var world = 34;");
    hvm_dump(&state.vm);

    hstate_exec_source(&state, "hello = 489 - hello + world;");
    hvm_dump(&state.vm);

    hstate_exec_source(&state, "var x = 0;");
    hstate_exec_source(&state, "while(x < 10) { x = x + 1; }");
    hvm_dump(&state.vm);

    // hstate_exec_source(&state, "if (hello == 420) { world = 69; }");
    // hvm_dump(&state.vm);

    // hstate_exec_source(&state, "func pow(a, b) { a * b; }");
    // hvm_dump(&state.vm);

    // This will be the standard library of hotaru
    // hstate_exec_source(&state, "bytes_create()");
    // hstate_exec_source(&state, "bytes_destroy()");
    // hstate_exec_source(&state, "load_bytes_from_file()");
    // hstate_exec_source(&state, "save_bytes_to_file()");
    // hstate_exec_source(&state, "bytes_is_little_endian()");
    // hstate_exec_source(&state, "bytes_push()");
    // hstate_exec_source(&state, "bytes_shift()");
    // hstate_exec_source(&state, "bytes_load_int()");
    // hstate_exec_source(&state, "bytes_push_int()");
    // hstate_exec_source(&state, "bytes_load_float()");
    // hstate_exec_source(&state, "bytes_push_float()");
    // hstate_exec_source(&state, "print_bytes_as_string()");
    // hstate_exec_source(&state, "print_bytes_as_hex()");
    // hstate_exec_source(&state, "print_int()");
    // hstate_exec_source(&state, "print_float()");
}

void test(void)
{
    hState state;
    hstate_init(&state);

    hExpr a;
    hExpr b;
    hExpr c;

    a.type = HEXPR_INT_LITERAL;
    a.as.int_literal = 34;

    b.type = HEXPR_INT_LITERAL;
    b.as.int_literal = 35;

    c.type = HEXPR_BINOP;
    c.as.binop.left = &a;
    c.as.binop.right = &b;

    hStmt stmt;

    stmt.type = HSTMT_VAR_INIT;
    stmt.as.var_init.name = SV("hello");
    stmt.as.var_init.value = a;
    hstate_exec_stmt(&state, &stmt);

    stmt.type = HSTMT_VAR_INIT;
    stmt.as.var_init.name = SV("world");
    stmt.as.var_init.value = b;
    hstate_exec_stmt(&state, &stmt);

    a.type = HEXPR_VAR_READ;
    a.as.var_read.name = SV("hello");
    stmt.type = HSTMT_VAR_ASSIGN;
    stmt.as.var_assign.name = SV("world");
    stmt.as.var_assign.value = c;
    hstate_exec_stmt(&state, &stmt);
}
