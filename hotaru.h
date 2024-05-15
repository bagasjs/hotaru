#ifndef HOTARU_H_
#define HOTARU_H_

#include <stdint.h>

#include "hvm.h"
#include "utils.h"

typedef enum hResult {
    HRES_OK = 0,
    HRES_INVALID_VARIABLE,
} hResult;

typedef enum hLogLevel {
    HLOG_FATAL = 0,
    HLOG_ERROR,
    HLOG_WARN,
    HLOG_INFO,

    COUNT_HLOG_LEVELS,
} hLogLevel;

typedef struct hPosition {
    uint32_t row, col;
} hPosition;

typedef struct hExpr hExpr;

typedef enum hBinOpType {
    HBINOP_NONE = 0,
    HBINOP_ADD,
    HBINOP_SUB,
    HBINOP_MUL,
    HBINOP_EQ,
    HBINOP_NE,
    HBINOP_GT,
    HBINOP_GE,
    HBINOP_LT,
    HBINOP_LE,
    COUNT_HBINOP_TYPES,
} hBinOpType;

typedef enum hExprType {
    HEXPR_NONE = 0,
    HEXPR_INT_LITERAL,
    HEXPR_FLOAT_LITERAL,
    HEXPR_BINOP,
    HEXPR_VAR_READ,
} hExprType;

typedef struct hBinOpExpr {
    hBinOpType type;
    hExpr *left;
    hExpr *right;
} hBinOpExpr;

struct hExpr {
    hExprType type;
    union {
        int64_t int_literal;
        double float_literal;
        hBinOpExpr binop;

        struct { 
            StringView name; 
        } var_read;
    } as;
};

typedef struct hStmt hStmt;

typedef struct hBlock {
    hStmt *items;
    uint32_t count;
    uint32_t capacity;
} hBlock;

typedef enum hStmtType {
    HSTMT_NONE = 0,
    HSTMT_VAR_INIT,
    HSTMT_VAR_ASSIGN,
    HSTMT_WHILE,
} hStmtType;

typedef struct hVarInitAndAssignStmt {
    StringView name;
    hExpr value;
} hVarInitAndAssignStmt;

typedef struct hWhileStmt {
    hExpr condition;
    hBlock body;
} hWhileStmt;

struct hStmt {
    hStmtType type;
    union {
        hVarInitAndAssignStmt var_init;
        hVarInitAndAssignStmt var_assign;
        hWhileStmt _while;
    } as;
};

typedef struct hVarBinding {
    StringView name;
    uint32_t pos;
} hVarBinding;

typedef struct hScope hScope;
struct hScope {
    hScope *prev;

    hVarBinding *items;
    ut_size count;
    ut_size capacity;
};

typedef struct hState {
    HVM vm;
    Arena arena;
    hScope global;

    HVM_Module mod;
    uint32_t vsp; // virtual stack pointer
    uint32_t vss; // virtual stack scope

    hScope *current;
} hState;

void hlog_message(hLogLevel level, const char *fmt, ...);

hVarBinding *hscope_find(const hScope *scope, StringView name);
hVarBinding *hscope_append(hScope *scope, hVarBinding binding, Arena *arena);

void hstate_init(hState *state);
void hstate_deinit(hState *state);
hResult hstate_exec_expr(hState *state, const hExpr *expr);
hResult hstate_exec_stmt(hState *state, const hStmt *stmt);
hResult hstate_exec_source(hState *state, const char *source);
hResult hstate_exec_file(hState *state, const char *filepath);

hResult hstate_compile_expr(hState *state, const hExpr *expr);
hResult hstate_compile_stmt(hState *state, const hStmt *stmt);
hResult hstate_compile_source(hState *state, const char *source);

#endif // HOTARU_H_
