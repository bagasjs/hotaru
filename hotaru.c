#include "hotaru.h"
#include "hvm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static const char *level_msg[COUNT_HLOG_LEVELS] = { "[FATAL]", "[ERROR]", "[WARN]", "[INFO]", };
static char format_buf[32*1024];

typedef struct hBinOpInfo {
    hBinOpType type;
    HVM_InstType inst;
} hBinOpInfo;

static const hBinOpInfo _binops_info[COUNT_HBINOP_TYPES] = {
[HBINOP_NONE] = { .type = HBINOP_NONE, .inst = HVM_INST_NONE, },
[HBINOP_ADD] = { .type = HBINOP_ADD, .inst = HVM_INST_ADD, },
[HBINOP_SUB] = { .type = HBINOP_SUB, .inst = HVM_INST_SUB, },
[HBINOP_MUL] = { .type = HBINOP_MUL, .inst = HVM_INST_MUL, },
[HBINOP_EQ] = { .type = HBINOP_EQ, .inst = HVM_INST_EQ, },
[HBINOP_NE] = { .type = HBINOP_NE, .inst = HVM_INST_NE, },
[HBINOP_LT] = { .type = HBINOP_LT, .inst = HVM_INST_LT, },
[HBINOP_LE] = { .type = HBINOP_LE, .inst = HVM_INST_LE, },
[HBINOP_GT] = { .type = HBINOP_GT, .inst = HVM_INST_GT, },
[HBINOP_GE] = { .type = HBINOP_GE, .inst = HVM_INST_GE, },
};

void hlog_message(hLogLevel level, const char *fmt, ...)
{
    FILE *f[COUNT_HLOG_LEVELS] = { stderr, stderr, stdout, stdout, };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(format_buf, sizeof(format_buf), fmt, ap);
    fprintf(f[level], "%s %s\n", level_msg[level], format_buf);
    va_end(ap);
    
    if(level == HLOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}

hVarBinding *hscope_append(hScope *scope, hVarBinding binding, Arena *arena)
{
    UT_ASSERT(scope);
    UT_ASSERT(arena);

    if(scope->count + 1 > scope->capacity) {
        ut_size new_capacity = scope->capacity * 2;
        if(new_capacity == 0) new_capacity = 32;

        hVarBinding *new_items = arena_malloc(arena, sizeof(hVarBinding)*new_capacity);
        ut_memcpy(new_items, scope->items, scope->capacity * sizeof(binding));
        scope->capacity = new_capacity;
        scope->items = new_items; // No need to deallocate the old items since this is in arena
    }

    hVarBinding *res = &scope->items[scope->count];
    *res = binding;
    scope->count += 1;
    return res;
}

hVarBinding *hscope_find(const hScope *scope, StringView name)
{
    hVarBinding *res = UT_NULL;
    while(scope != UT_NULL && res == UT_NULL) {
        for(ut_size i = 0; i < scope->count; ++i) {
            hVarBinding *cur = &scope->items[i];
            if(sv_eq(cur->name, name)) 
                res = cur;
        }
        scope = scope->prev;
    }
    return res;
}

void hstate_init(hState *state)
{
    UT_ASSERT(state);

    hvm_init(&state->vm);
    hvm_module_init(&state->mod);
    state->arena.begin = 0;
    state->arena.end = 0;
    state->global.prev = UT_NULL;
    state->global.count = 0;
    state->global.capacity = 0;
    state->global.items = 0;
    state->current = &state->global;

    state->vsp = 0;
    state->vss = 0;
}

void hstate_deinit(hState *state)
{
    arena_free(&state->arena);
    hvm_module_deinit(&state->mod);
}

hResult hstate_compile_expr(hState *state, const hExpr *expr)
{
    UT_ASSERT(state);
    UT_ASSERT(expr);

    switch(expr->type) {
        case HEXPR_INT_LITERAL:
            {
                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_PUSH, 
                            HVM_WORD_I64(expr->as.int_literal)));
                state->vsp += 1;
            } break;
        case HEXPR_BINOP:
            {
                hstate_compile_expr(state, expr->as.binop.left);
                hstate_compile_expr(state, expr->as.binop.right);
                hBinOpInfo info = _binops_info[expr->as.binop.type];
                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            info.inst, 
                            HVM_NULL_WORD));
                state->vsp -= 1;
            } break;
        case HEXPR_VAR_READ:
            {
                hVarBinding *var = hscope_find(state->current, expr->as.var_read.name);
                if(!var) return HRES_INVALID_VARIABLE;
                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_COPYABS, 
                            HVM_WORD_U64(var->pos)));
                state->vsp += 1;
            } break;
        default:
            UT_ASSERT(0 && "Unreachable expr in hstate_compile_expr()");
            break;
    }

    return 0;
}

hResult hstate_exec_expr(hState *state, const hExpr *expr)
{
    UT_ASSERT(state);
    UT_ASSERT(expr);

    HVM_Inst inst;
    switch(expr->type) {
        case HEXPR_INT_LITERAL:
            {
                inst.type = HVM_INST_PUSH;
                inst.op.as_i64 = expr->as.int_literal; 
                hvm_exec(&state->vm, inst);
            } break;
        case HEXPR_BINOP:
            {
                hstate_exec_expr(state, expr->as.binop.left);
                hstate_exec_expr(state, expr->as.binop.right);

                hBinOpInfo info = _binops_info[expr->as.binop.type];
                inst.type = info.inst;
                hvm_exec(&state->vm, inst);
            } break;
        case HEXPR_VAR_READ:
            {
                hVarBinding *var = hscope_find(state->current, expr->as.var_read.name);
                if(!var) return HRES_INVALID_VARIABLE;
                inst.type = HVM_INST_COPYABS;
                inst.op.as_u64 = var->pos;
                hvm_exec(&state->vm, inst);
            } break;
        default:
            UT_ASSERT(0 && "Unreachable expr in hstate_exec_expr()");
            break;
    }

    return HRES_OK;
}

hResult hstate_compile_block(hState *state, const hBlock block)
{
    hvm_module_append(&state->mod, HVM_MAKE_INST(
                HVM_INST_BEGIN_SCOPE,
                HVM_NULL_WORD));

    for(uint32_t i = 0; i < block.count; ++i) {
        hstate_compile_stmt(state, &block.items[i]);
    }

    hvm_module_append(&state->mod, HVM_MAKE_INST(
                HVM_INST_END_SCOPE,
                HVM_NULL_WORD));
    return HRES_OK;
}

hResult hstate_compile_stmt(hState *state, const hStmt *stmt)
{
    UT_ASSERT(state);
    UT_ASSERT(stmt);

    switch(stmt->type) {
        case HSTMT_VAR_INIT:
            {
                uint32_t last_sp = state->vsp;
                hResult res = hstate_compile_expr(state, &stmt->as.var_init.value);
                if(res != HRES_OK) return res;
                hVarBinding var;
                var.name = stmt->as.var_init.name;
                var.pos = last_sp;
                hscope_append(state->current, var, &state->arena);
            } break;
        case HSTMT_VAR_ASSIGN:
            {
                hVarBinding *var = hscope_find(state->current, stmt->as.var_assign.name);
                if(!var) return HRES_INVALID_VARIABLE;

                hResult res = hstate_compile_expr(state, &stmt->as.var_init.value);
                if(res != HRES_OK) return res;

                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_SWAPABS,
                            HVM_WORD_U64(var->pos)));
                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_POP,
                            HVM_NULL_WORD));
            } break;
        case HSTMT_IF:
            {
                // HACK: (1) first we jump to the start of if's condition
                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_JMP,
                            HVM_WORD_U64(state->mod.count + 2)));
                // HACK: (2) we will jump here if body of a condition is done
                uint32_t body_completed_jump_target = state->mod.count;
                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_JMP,
                            HVM_NULL_WORD));
                HVM_Inst *inst = &state->mod.items[body_completed_jump_target];

                HVM_Module prev_mod = state->mod;
                hvm_module_init(&state->mod);
                hstate_compile_expr(state, &stmt->as._if.condition);
                hstate_compile_block(state, stmt->as._if.body);
                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_JMP,
                            HVM_WORD_U64(body_completed_jump_target)));
                for(uint32_t i =  0; i < stmt->as._if._elif.count; ++i) {
                    hElifBlock elif = stmt->as._if._elif.items[i];
                    hstate_compile_expr(state, &elif.condition);
                    hstate_compile_block(state, elif.body);
                    hvm_module_append(&state->mod, HVM_MAKE_INST(
                                HVM_INST_JMP,
                                HVM_WORD_U64(body_completed_jump_target)));
                }
                hstate_compile_block(state, stmt->as._if._else);
                for(uint32_t i = 0; i < state->mod.count; ++i) 
                    hvm_module_append(&prev_mod, state->mod.items[i]);
                inst->op = HVM_WORD_U64(prev_mod.count);
                hvm_module_deinit(&state->mod);
                state->mod = prev_mod;
            } break;
        case HSTMT_WHILE:
            {
                HVM_Module body_mod;
                hvm_module_init(&body_mod);
                HVM_Module prev_mod = state->mod;
                state->mod = body_mod;
                hBlock body = stmt->as._while.body;
                for(uint32_t i = 0; i < body.count; ++i) {
                    hstate_compile_stmt(state, &body.items[i]);
                }
                body_mod = state->mod;
                state->mod = prev_mod;

                {
                    hvm_module_append(&state->mod, HVM_MAKE_INST(
                                HVM_INST_BEGIN_SCOPE,
                                HVM_NULL_WORD));
                    uint32_t while_loop_start_pc = state->mod.count;
                    hstate_compile_expr(state, &stmt->as._while.condition);

                    uint32_t count_insts_before_end_scope = body_mod.count + 2;
                    uint32_t while_loop_finish_pc = state->mod.count + count_insts_before_end_scope; 
                    hvm_module_append(&state->mod, HVM_MAKE_INST(
                                HVM_INST_JZ,
                                HVM_WORD_U64(while_loop_finish_pc)));

                    for(uint32_t i = 0; i < body_mod.count; ++i) 
                        hvm_module_append(&state->mod, body_mod.items[i]);

                    hvm_module_append(&state->mod, HVM_MAKE_INST(
                                HVM_INST_JMP,
                                HVM_WORD_U64(while_loop_start_pc)));
                    hvm_module_append(&state->mod, HVM_MAKE_INST(
                                HVM_INST_END_SCOPE,
                                HVM_NULL_WORD));
                }
                hvm_module_deinit(&body_mod);
            } break;

        case HSTMT_DUMP:
            {
                hResult res = hstate_compile_expr(state, &stmt->as.dump);
                if(res != HRES_OK) return res;

                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_DUMP,
                            HVM_NULL_WORD));
            } break;
        default:
            {
                UT_ASSERT(0 && "Unreachable stmt");
            } break;
    }

    return 0;
}

hResult hstate_exec_stmt(hState *state, const hStmt *stmt)
{
    UT_ASSERT(state);
    UT_ASSERT(stmt);

    switch(stmt->type) {
        case HSTMT_VAR_INIT:
            {
                uint32_t last_sp = state->vm.sp;
                hResult res = hstate_exec_expr(state, &stmt->as.var_init.value);
                if(res != HRES_OK) return res;
                hVarBinding var;
                var.name = stmt->as.var_init.name;
                var.pos = last_sp;
                hscope_append(state->current, var, &state->arena);
            } break;
        case HSTMT_VAR_ASSIGN:
            {
                HVM_Inst inst;

                hVarBinding *var = hscope_find(state->current, stmt->as.var_assign.name);
                if(!var) return HRES_INVALID_VARIABLE;

                hResult res = hstate_exec_expr(state, &stmt->as.var_assign.value);
                if(res != HRES_OK) return res;

                inst.type = HVM_INST_SWAPABS;
                inst.op.as_u64 = var->pos;
                hvm_exec(&state->vm, inst);

                inst.type = HVM_INST_POP;
                hvm_exec(&state->vm, inst);
            } break;

        case HSTMT_WHILE:
        case HSTMT_IF:
            {
                hvm_module_init(&state->mod);
                state->mod.count = 0;
                hstate_compile_stmt(state, stmt);
                hvm_module_append(&state->mod, HVM_MAKE_INST(
                            HVM_INST_HALT,
                            HVM_NULL_WORD));
                uint32_t last_pc = state->vm.pc;
                state->vm.pc = 0;
                hvm_exec_module(&state->vm, state->mod);
                state->vm.halt = ut_false;
                state->vm.pc = last_pc;
                hvm_module_deinit(&state->mod);
            } break;

        case HSTMT_DUMP:
            {
                hResult res = hstate_exec_expr(state, &stmt->as.dump);
                if(res != HRES_OK) return res;

                hvm_exec(&state->vm, HVM_MAKE_INST(
                            HVM_INST_DUMP,
                            HVM_NULL_WORD));
            } break;
        default:
            {
                UT_ASSERT(0 && "Unreachable expr in hstate_exec_stmt()");
            } break;
    }

    return HRES_OK;
}

