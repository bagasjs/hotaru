#include "hotaru.h"
#include "utils.h"
#include <stdio.h>

#define HLEXER_CACHE_CAPACITY 32

typedef enum hTokenType {
    HTOKEN_NONE = 0,
    HTOKEN_IDENTIFIER,
    HTOKEN_INT_LITERAL,
    HTOKEN_FLOAT_LITERAL,

    HTOKEN_SEMICOLON,
    HTOKEN_LPAREN,
    HTOKEN_RPAREN,
    HTOKEN_LCURLY,
    HTOKEN_RCURLY,

    HTOKEN_ASSIGN,
    HTOKEN_PLUS,
    HTOKEN_MINUS,
    HTOKEN_ASTERISK,

    HTOKEN_EQ,
    HTOKEN_NE,
    HTOKEN_GT,
    HTOKEN_LT,
    HTOKEN_GE,
    HTOKEN_LE,

    HTOKEN_VAR,
    HTOKEN_WHILE,
    HTOKEN_BREAK,
    HTOKEN_CONTINUE,
    HTOKEN_DEBUG_DUMP,

    COUNT_HTOKENS,
} hTokenType;

typedef struct hTokenInfo {
    const char *view;
    ut_bool is_binop;
    hBinOpType binop;
} hTokenInfo;

static hTokenInfo _token_infos[COUNT_HTOKENS] = {
    [HTOKEN_IDENTIFIER] = { .view = "identifier", .is_binop = ut_false,  },
    [HTOKEN_INT_LITERAL] = { .view = "integer literal", .is_binop = ut_false, },
    [HTOKEN_FLOAT_LITERAL] = { .view = "float literal", .is_binop = ut_false, },

    [HTOKEN_SEMICOLON] = { .view = ";", .is_binop = ut_false, },
    [HTOKEN_LPAREN] = { .view = "(", .is_binop = ut_false, },
    [HTOKEN_RPAREN] = { .view = ")", .is_binop = ut_false, },
    [HTOKEN_LCURLY] = { .view = "{", .is_binop = ut_false, },
    [HTOKEN_RCURLY] = { .view = "}", .is_binop = ut_false, },

    [HTOKEN_ASSIGN] = { .view = "=", .is_binop = ut_false, },
    [HTOKEN_PLUS] = { .view = "+", .is_binop = ut_true, .binop = HBINOP_ADD,},
    [HTOKEN_MINUS] = { .view = "-", .is_binop = ut_true, .binop = HBINOP_SUB, },
    [HTOKEN_ASTERISK] = { .view = "*", .is_binop = ut_true, .binop = HBINOP_MUL, },
    [HTOKEN_EQ] = { .view = "==", .is_binop = ut_true, .binop = HBINOP_EQ, },
    [HTOKEN_NE] = { .view = "!=", .is_binop = ut_true, .binop = HBINOP_NE, },
    [HTOKEN_GT] = { .view = ">", .is_binop = ut_true, .binop = HBINOP_GT, },
    [HTOKEN_GE] = { .view = ">=", .is_binop = ut_true, .binop = HBINOP_GE, },
    [HTOKEN_LT] = { .view = "<", .is_binop = ut_true, .binop = HBINOP_LT, },
    [HTOKEN_LE] = { .view = "<=", .is_binop = ut_true, .binop = HBINOP_LE, },

    [HTOKEN_VAR] = { .view = "var", .is_binop = ut_false, },
    [HTOKEN_WHILE] = { .view = "while", .is_binop = ut_false, },
    [HTOKEN_BREAK] = { .view = "break", .is_binop = ut_false, },
    [HTOKEN_CONTINUE] = { .view = "continue", .is_binop = ut_false, },
    [HTOKEN_DEBUG_DUMP] = { .view = "dd", .is_binop = ut_false, },
};

typedef struct hToken {
    hTokenType type;
    StringView literal;
    hPosition pos;
} hToken;

typedef struct hLexer {
    StringView source;
    ut_size i;
    char cc;
    char pc;

    hPosition cpos;
    struct {
        hToken items[HLEXER_CACHE_CAPACITY];
        ut_uint32 head, tail;
        ut_bool carry;
    } cache;
} hLexer;

static inline ut_uint32 hlexer_cache_count(hLexer *lex)
{
    return (lex->cache.head + HLEXER_CACHE_CAPACITY - lex->cache.tail) % HLEXER_CACHE_CAPACITY;
}

ut_bool hlexer_cache_push(hLexer *lex, hToken item)
{
    ut_uint32 index = lex->cache.head;
    if(index + 1 >= HLEXER_CACHE_CAPACITY) {
        if(lex->cache.carry) {
            // ERROR: cache is full
            return ut_false;
        }

        lex->cache.carry = ut_true;
        lex->cache.head = 0;
    } else {
        lex->cache.head += 1;
    }
    lex->cache.items[index] = item;
    return ut_true;
}

ut_bool hlexer_cache_shift(hLexer *lex, hToken *item)
{
    ut_uint32 index = lex->cache.tail;
    if(index + 1 >= HLEXER_CACHE_CAPACITY) {
        if(!lex->cache.carry) {
            // ERROR: cache is empty
            return ut_false;
        }

        lex->cache.carry = ut_false;
        lex->cache.tail = 0;
    } else {
        lex->cache.tail += 1;
    }
    *item = lex->cache.items[index];
    return ut_true;
}

ut_bool hlexer_cache_extend(hLexer *lex, hTokenType type, StringView literal)
{
    hToken tok;
    tok.type = type;
    tok.literal = literal;
    tok.pos = lex->cpos;
    return hlexer_cache_push(lex, tok);
}

void hlexer_advance(hLexer *lex)
{
    lex->i += 1;
    lex->cc = lex->source.data[lex->i];
    lex->pc = lex->i + 1 < lex->source.count ? lex->source.data[lex->i + 1] : 0;
    lex->cpos.col += 1;
}

ut_bool hlexer_cache_next(hLexer *lex)
{
    while(ut_isspace(lex->cc)) {
        if(lex->cc == '\n') {
            lex->cpos.row += 1;
            lex->cpos.col = 0;
        }
        hlexer_advance(lex);
    }

    switch(lex->cc) {
        case '\0':
            {
                return ut_false;
            } break;
        case '{':
            {
                hlexer_cache_extend(lex, HTOKEN_LCURLY, sv_slice(lex->source, lex->i, lex->i + 1));
                hlexer_advance(lex);
            } break;
        case '}':
            {
                hlexer_cache_extend(lex, HTOKEN_RCURLY, sv_slice(lex->source, lex->i, lex->i + 1));
                hlexer_advance(lex);
            } break;
        case '(':
            {
                hlexer_cache_extend(lex, HTOKEN_LPAREN, sv_slice(lex->source, lex->i, lex->i + 1));
                hlexer_advance(lex);
            } break;
        case ')':
            {
                hlexer_cache_extend(lex, HTOKEN_RPAREN, sv_slice(lex->source, lex->i, lex->i + 1));
                hlexer_advance(lex);
            } break;
        case ';':
            {
                hlexer_cache_extend(lex, HTOKEN_SEMICOLON, sv_slice(lex->source, lex->i, lex->i + 1));
                hlexer_advance(lex);
            } break;
        case '+':
            {
                hlexer_cache_extend(lex, HTOKEN_PLUS, sv_slice(lex->source, lex->i, lex->i + 1));
                hlexer_advance(lex);
            } break;
        case '-':
            {
                hlexer_cache_extend(lex, HTOKEN_MINUS, sv_slice(lex->source, lex->i, lex->i + 1));
                hlexer_advance(lex);
            } break;
        case '*':
            {
                hlexer_cache_extend(lex, HTOKEN_ASTERISK, sv_slice(lex->source, lex->i, lex->i + 1));
                hlexer_advance(lex);
            } break;
        case '!':
            {
                ut_size start = lex->i;
                hlexer_advance(lex);
                if(lex->cc == '=') {
                    hlexer_cache_extend(lex, HTOKEN_NE, sv_slice(lex->source, start, lex->i));
                    hlexer_advance(lex);
                } else {
                    hlog_message(HLOG_FATAL, "Invalid syntax `!%c`", lex->cc);
                }
            } break;
        case '>':
            {
                ut_size start = lex->i;
                hlexer_advance(lex);
                if(lex->cc == '=') {
                    hlexer_cache_extend(lex, HTOKEN_GE, sv_slice(lex->source, start, lex->i));
                } else {
                    hlexer_cache_extend(lex, HTOKEN_GT, sv_slice(lex->source, start, lex->i));
                    hlexer_advance(lex);
                }
            } break;
        case '<':
            {
                ut_size start = lex->i;
                hlexer_advance(lex);
                if(lex->cc == '=') {
                    hlexer_cache_extend(lex, HTOKEN_LE, sv_slice(lex->source, start, lex->i));
                } else {
                    hlexer_cache_extend(lex, HTOKEN_LT, sv_slice(lex->source, start, lex->i));
                    hlexer_advance(lex);
                }
            } break;
        case '=':
            {
                ut_size start = lex->i;
                hlexer_advance(lex);
                if(lex->cc == '=') {
                    hlexer_cache_extend(lex, HTOKEN_EQ, sv_slice(lex->source, start, lex->i));
                    hlexer_advance(lex);
                } else {
                    hlexer_cache_extend(lex, HTOKEN_ASSIGN, sv_slice(lex->source, start, lex->i));
                }
            } break;
        default:
            {
                if(ut_isalpha(lex->cc)) {
                    ut_size start = lex->i;
                    while(ut_isalnum(lex->cc) || lex->cc== '_') {
                        hlexer_advance(lex);
                    }

                    StringView name = sv_slice(lex->source, start, lex->i);
                    if(sv_eq(name, SV("var"))) {
                        hlexer_cache_extend(lex, HTOKEN_VAR, name);
                    } else if(sv_eq(name, SV("while"))) {
                        hlexer_cache_extend(lex, HTOKEN_WHILE, name);
                    } else if(sv_eq(name, SV("break"))) {
                        hlexer_cache_extend(lex, HTOKEN_BREAK, name);
                    } else if(sv_eq(name, SV("continue"))) {
                        hlexer_cache_extend(lex, HTOKEN_CONTINUE, name);
                    } else {
                        hlexer_cache_extend(lex, HTOKEN_IDENTIFIER, name);
                    }

                } else if(ut_isdigit(lex->cc)) {
                    ut_size start = lex->i;
                    ut_bool floating_point = ut_false;
                    while(ut_isdigit(lex->cc) || lex->cc == '.') {
                        if(lex->cc == '.') {
                            if(floating_point) {
                                hlog_message(HLOG_FATAL, "There's should not be another dot in already floating point token");
                            }
                            floating_point = ut_true;
                        }
                        hlexer_advance(lex);
                    }
                    hlexer_cache_extend(lex, floating_point ? HTOKEN_FLOAT_LITERAL : HTOKEN_INT_LITERAL, 
                            sv_slice(lex->source, start, lex->i));
                } else {
                    hlexer_cache_extend(lex, HTOKEN_NONE, sv_slice(lex->source, lex->i, 1));
                }
            } break;
    }

    return ut_true;
}

void hlexer_init(hLexer *lex, const char *source)
{
    UT_ASSERT(lex);
    UT_ASSERT(source);
    lex->source = sv_from_cstr(source);
    lex->i = 0;
    lex->cc = lex->source.data[lex->i];
    lex->pc = lex->i + 1 < lex->source.count ? lex->source.data[lex->i + 1] : 0;
    lex->cache.carry = ut_false;
    lex->cache.head = 0;
    lex->cache.tail = 0;
}

ut_bool hlexer_next(hLexer *lex, hToken *token)
{
    if(hlexer_cache_count(lex) == 0) {
        if(!hlexer_cache_next(lex)) {
            return ut_false;
        }
    }
    return hlexer_cache_shift(lex, token);
}

ut_bool hlexer_peek(hLexer *lex, hToken *token, ut_size index)
{
    while(hlexer_cache_count(lex) <= index) {
        if(!hlexer_cache_next(lex)) {
            return ut_false;
        }
    }

    if(lex->cache.head == 0) {
        index = HLEXER_CACHE_CAPACITY - 1;
    } else {
        index = lex->cache.head - 1;
    }
    *token = lex->cache.items[index];
    return ut_true;
}

hToken hlexer_expect_token(hLexer *lex, hTokenType expected_type)
{
    hToken tok;
    if(!hlexer_next(lex, &tok)) {
        hlog_message(HLOG_FATAL, "Expecting a token `%s` but there's nothing at %u,%u", 
                _token_infos[expected_type].view,
                lex->cpos.row, lex->cpos.col);
    }
    if(tok.type != expected_type) {
        hlog_message(HLOG_FATAL, "Expecting token `%s` but found `%s`", 
                _token_infos[expected_type].view, _token_infos[tok.type].view);
    }
    return tok;
}

hExpr hparse_expr(Arena *a, hLexer *lex)
{
    hToken token;
    hExpr res;
    ut_memset(&res, 0, sizeof(res));

    if(!hlexer_peek(lex, &token, 0)) {
        hlog_message(HLOG_FATAL, "There's no token to parse an expression on");
    }

    switch(token.type) {
        case HTOKEN_IDENTIFIER:
            {
                token = hlexer_expect_token(lex, HTOKEN_IDENTIFIER);
                hToken ntok;
                ntok.type = HTOKEN_NONE;
                hlexer_peek(lex, &ntok, 0);
                if(ntok.type == HTOKEN_LPAREN) {
                    // TODO: function
                    hlog_message(HLOG_FATAL, "Function call expression is not implemented yet");
                } else {
                    res.type = HEXPR_VAR_READ;
                    res.as.var_read.name = token.literal;
                }

                hlexer_peek(lex, &ntok, 0);
                if(_token_infos[ntok.type].is_binop) {
                    hExpr left = res;
                    res.type = HEXPR_BINOP;
                    res.as.binop.type = _token_infos[ntok.type].binop;

                    res.as.binop.left = arena_malloc(a, sizeof(hExpr));
                    UT_ASSERT(res.as.binop.left);
                    *res.as.binop.left = left;
                    hlexer_next(lex, &ntok); // skip the binop token

                    res.as.binop.right = arena_malloc(a, sizeof(hExpr));
                    UT_ASSERT(res.as.binop.right);
                    *res.as.binop.right = hparse_expr(a, lex);
                }
            } break;
        case HTOKEN_INT_LITERAL:
            {
                token = hlexer_expect_token(lex, HTOKEN_INT_LITERAL);
                res.type = HEXPR_INT_LITERAL;
                res.as.int_literal = sv_to_int(token.literal);

                hToken ntok;
                hlexer_peek(lex, &ntok, 0);
                if(_token_infos[ntok.type].is_binop) {
                    hExpr left = res;
                    res.type = HEXPR_BINOP;
                    res.as.binop.type = _token_infos[ntok.type].binop;

                    res.as.binop.left = arena_malloc(a, sizeof(hExpr));
                    UT_ASSERT(res.as.binop.left);
                    *res.as.binop.left = left;
                    hlexer_next(lex, &ntok); // skip the binop token

                    res.as.binop.right = arena_malloc(a, sizeof(hExpr));
                    UT_ASSERT(res.as.binop.right);
                    *res.as.binop.right = hparse_expr(a, lex);
                }
            } break;
        default:
            {
                hlog_message(HLOG_FATAL, "Unreachable in hparse_expr() due to token `%s`", _token_infos[token.type].view);
            } break;
    }

    return res;
}

hBlock hparse_block(Arena *a, hLexer *lex);

hStmt hparse_stmt(Arena *a, hLexer *lex)
{
    hToken token;
    hStmt res;
    ut_memset(&res, 0, sizeof(res));

    if(!hlexer_peek(lex, &token, 0)) {
        hlog_message(HLOG_FATAL, "There's no token to parse a statement on");
    }

    switch(token.type) {
        case HTOKEN_VAR:
            {
                token = hlexer_expect_token(lex, HTOKEN_VAR);
                StringView name = hlexer_expect_token(lex, HTOKEN_IDENTIFIER).literal;
                hlexer_expect_token(lex, HTOKEN_ASSIGN);

                res.type = HSTMT_VAR_INIT;
                res.as.var_init.name = name;
                res.as.var_init.value = hparse_expr(a, lex);
                hlexer_expect_token(lex, HTOKEN_SEMICOLON);
            } break;
        case HTOKEN_IDENTIFIER:
            {
                token = hlexer_expect_token(lex, HTOKEN_IDENTIFIER);
                hToken ntok;
                hlexer_next(lex, &ntok);
                if(ntok.type == HTOKEN_ASSIGN) {
                    res.type = HSTMT_VAR_ASSIGN;
                    res.as.var_assign.name = token.literal;
                    res.as.var_assign.value = hparse_expr(a, lex);
                }
                hlexer_expect_token(lex, HTOKEN_SEMICOLON);
            } break;
        case HTOKEN_WHILE:
            {
                token = hlexer_expect_token(lex, HTOKEN_WHILE);
                res.type = HSTMT_WHILE;
                hlexer_expect_token(lex, HTOKEN_LPAREN);
                res.as._while.condition = hparse_expr(a, lex);
                hlexer_expect_token(lex, HTOKEN_RPAREN);
                res.as._while.body = hparse_block(a, lex);
            } break;
        default:
            {
                hlog_message(HLOG_FATAL, "Unreachable in hparse_stmt() due to token `%s`", _token_infos[token.type].view);
            } break;
    }

    return res;
}

void hblock_push_stmt(hBlock *block, Arena *a, hStmt stmt)
{
    UT_ASSERT(block);
    UT_ASSERT(a);

    if(block->count + 1 > block->capacity) {
        ut_size new_capacity = block->capacity * 2;
        if(new_capacity == 0) new_capacity = 32;

        hStmt *new_items = arena_malloc(a, sizeof(stmt)*new_capacity);
        ut_memcpy(new_items, block->items, block->capacity * sizeof(stmt));
        block->capacity = new_capacity;
        block->items = new_items; // No need to deallocate the old items since this is in arena
    }

    block->items[block->count] = stmt;
    block->count += 1;
}

hBlock hparse_block(Arena *a, hLexer *lex)
{
    hBlock res;
    res.count = 0;
    res.capacity = 0;
    hlexer_expect_token(lex, HTOKEN_LCURLY);
    hToken token;
    while(token.type != HTOKEN_RCURLY) {
        hStmt stmt = hparse_stmt(a, lex);
        hblock_push_stmt(&res, a, stmt);
        if(!hlexer_peek(lex, &token, 0)) {
            hlog_message(HLOG_FATAL, "Expected another statement or '}' token but reached end of file");
        }
    }
    hlexer_expect_token(lex, HTOKEN_RCURLY);
    return res;
}

hResult hstate_exec_source(hState *state, const char *source)
{
    hLexer lex;
    Arena a;
    ut_memset(&a, 0, sizeof(a));

    hlexer_init(&lex, source);
    hStmt stmt = hparse_stmt(&a, &lex);
    hstate_exec_stmt(state, &stmt);
    return HRES_OK;
}

hResult hstate_exec_file(hState *state, const char *filepath)
{
    UT_UNUSED(state);
    UT_UNUSED(filepath);
    return HRES_OK;
}
