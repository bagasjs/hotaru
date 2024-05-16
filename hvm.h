#ifndef HVM_H_
#define HVM_H_

#define HVM_VERSION UT_MAKE_VERSION(0, 1, 0)
#define HVM_MAGIC_NUMBER 0xFBADF00D

#define HVM_STATIC_MEMORY_REGION_START
#define HVM_STATIC_MEMORY_REGION_CAPACITY 
#define HVM_GENERAL_USAGE_MEMORY_REGION_START
#define HVM_GENERAL_USAGE_MEMORY_REGION_CAPACITY

#include "utils.h"
#include <stdint.h>

#ifndef HVM_ASSERT
#include <assert.h>
#define HVM_ASSERT assert
#endif

#if !defined(HVM_MALLOC) && !defined(HVM_FREE)
#include <stdlib.h>
#define HVM_MALLOC malloc
#define HVM_FREE free
#endif

#if !defined(HVM_MALLOC) || !defined(HVM_FREE)
#error "Please define the HVM_MALLOC or HVM_FREE macro"
#endif

#ifndef HVM_STACK_CAPACITY
#define HVM_STACK_CAPACITY (1*1024)
#endif

#ifndef HVM_HEAP_CAPACITY
#define HVM_HEAP_CAPACITY (100*1024)
#endif

typedef struct HVM HVM;

typedef union HVM_Word {
    uint64_t as_u64;
    int64_t  as_i64;
    double   as_f64;
} HVM_Word;

#define HVM_NULL_WORD UT_LITERAL(HVM_Word){0}
#define HVM_WORD_U64(V) UT_LITERAL(HVM_Word){ .as_u64 = (V), }
#define HVM_WORD_I64(V) UT_LITERAL(HVM_Word){ .as_i64 = (V), }

typedef enum HVM_Trap{
    HVM_TRAP_NONE = 0,
    HVM_TRAP_INVALID_INSTRUCTION,
    HVM_TRAP_STACK_UNDERFLOW,
    HVM_TRAP_STACK_OVERFLOW,
} HVM_Trap;

typedef enum HVM_InstType {
    HVM_INST_NONE = 0,

    HVM_INST_HALT,
    HVM_INST_BEGIN_SCOPE,
    HVM_INST_END_SCOPE,

    // Copy the top stack value of current scope
    HVM_INST_COPY, 
    // Swap with the top stack value of current scope
    HVM_INST_SWAP, 
    // Copy the bottom stack value of current scope
    HVM_INST_BCOPY, 
    // Swap with the bottom stack value of current scope
    HVM_INST_BSWAP, 
    // Copy the bottom stack value regardless the current scope
    HVM_INST_COPYABS,
    // Swap with the bottom stack value regardless the current scope
    HVM_INST_SWAPABS, 

    HVM_INST_POP,

    HVM_INST_PUSH,
    HVM_INST_ADD,
    HVM_INST_SUB,
    HVM_INST_MUL,
    HVM_INST_EQ,
    HVM_INST_NE,
    HVM_INST_LT,
    HVM_INST_GT,
    HVM_INST_LE,
    HVM_INST_GE,

    HVM_INST_FPUSH,
    HVM_INST_FADD,
    HVM_INST_FSUB,
    HVM_INST_FMUL,
    HVM_INST_FEQ,
    HVM_INST_FNE,
    HVM_INST_FGT,
    HVM_INST_FGE,
    HVM_INST_FLT,
    HVM_INST_FLE,

    HVM_INST_JMP,
    HVM_INST_JZ,
    HVM_INST_JN,

    HVM_INST_DUMP,

    COUNT_HVM_INSTS,
} HVM_InstType;

typedef struct HVM_Inst {
    HVM_InstType type;
    HVM_Word op;
} HVM_Inst;

#define HVM_MAKE_INST(T, O) UT_LITERAL(HVM_Inst){ .type=(T), .op=(O), }

typedef struct HVM_InstInfo {
    HVM_InstType type;
    const char *name;
    ut_bool has_operand;
    int8_t min_sp;
    int8_t chg_sp;
} HVM_InstInfo;

typedef void (*HVM_NativeWrapperFn)(HVM *vm);

typedef struct HVM_NativeInfo {
    const char *sym;
    const char *name;
    int8_t min_sp;
    int8_t chg_sp;
    HVM_NativeWrapperFn wrapper;
} HVM_NativeInfo;

typedef struct HVM_Module {
    HVM_Inst *items;
    uint32_t count;
    uint32_t capacity;

    Buffer static_data;
} HVM_Module;

typedef struct HVM {
    HVM_Word stack[HVM_STACK_CAPACITY];
    uint32_t sp;
    uint32_t ss; // stack stride or stack start
    uint32_t pc;

    uint8_t halt;
    uint8_t heap[HVM_HEAP_CAPACITY];
} HVM;

typedef struct HVM_ModuleFileHeader {
    uint32_t magic_number;
    uint32_t version;
    uint32_t insts_amount;
    uint64_t program_start;
    uint64_t program_size;
    uint64_t static_data_start;
    uint64_t static_data_size;
} HVM_ModuleFileHeader;

void hvm_init(HVM *vm);
HVM_Trap hvm_exec(HVM *vm, HVM_Inst inst);
void hvm_dump(const HVM *vm);

void hvm_module_init(HVM_Module *module);
void hvm_module_deinit(HVM_Module *module);
void hvm_module_dump(const HVM_Module module);
void hvm_module_append(HVM_Module *module, HVM_Inst inst);
HVM_Trap hvm_exec_module(HVM *vm, const HVM_Module module);
ut_bool hvm_module_save_to_file(const HVM_Module module, const char *file_path);
ut_bool hvm_module_load_from_file(HVM_Module *module, const char *file_path, Arena *a);

#endif // HVM_H_
