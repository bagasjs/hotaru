#include "hvm.h"

#include "utils.h"

#include <stdio.h>
#ifndef HVM_NO_TRACE 
#define TRACE_PRINTF(...) printf(__VA_ARGS__)
#else
#define TRACE_PRINTF(...)
#endif

static HVM_InstInfo _inst_infos[COUNT_HVM_INSTS] = {
    [HVM_INST_NONE] = { .type = HVM_INST_NONE, .name = "(none)", .has_operand = ut_false, .min_sp = 0,  },

    [HVM_INST_HALT] = { .type = HVM_INST_HALT, .name = "halt", .has_operand = ut_false, .min_sp = 0,  },
    [HVM_INST_BEGIN_SCOPE] = { .type = HVM_INST_BEGIN_SCOPE, .name = "begin_scope", .has_operand = ut_false, .min_sp = 0, },
    [HVM_INST_END_SCOPE] = { .type = HVM_INST_END_SCOPE, .name = "end_scope", .has_operand = ut_false, .min_sp = 0, },

    [HVM_INST_POP] = { .type = HVM_INST_POP, .name = "pop", .has_operand = ut_false, .min_sp = 1, },
    [HVM_INST_COPY] = { .type = HVM_INST_COPY, .name = "copy", .has_operand = ut_true, .min_sp = 0, },
    [HVM_INST_SWAP] = { .type = HVM_INST_SWAP, .name = "swap", .has_operand = ut_true, .min_sp = 0, },
    [HVM_INST_BCOPY] = { .type = HVM_INST_BCOPY, .name = "bcopy", .has_operand = ut_true, .min_sp = 0, },
    [HVM_INST_BSWAP] = { .type = HVM_INST_BSWAP, .name = "bswap", .has_operand = ut_true, .min_sp = 0, },
    [HVM_INST_COPYABS] = { .type = HVM_INST_COPYABS, .name = "copyabs", .has_operand = ut_true, .min_sp = 0, },
    [HVM_INST_SWAPABS] = { .type = HVM_INST_SWAPABS, .name = "swapabs", .has_operand = ut_true, .min_sp = 0, },

    [HVM_INST_PUSH] = { .type = HVM_INST_PUSH, .name = "push", .has_operand = ut_true, .min_sp = 0, },
    [HVM_INST_ADD] = { .type = HVM_INST_ADD, .name = "add", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_SUB] = { .type = HVM_INST_SUB, .name = "sub", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_MUL] = { .type = HVM_INST_MUL, .name = "mul", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_EQ] = { .type = HVM_INST_EQ, .name = "eq", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_NE] = { .type = HVM_INST_NE, .name = "ne", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_GT] = { .type = HVM_INST_GT, .name = "gt", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_GE] = { .type = HVM_INST_GE, .name = "ge", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_LT] = { .type = HVM_INST_LT, .name = "lt", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_LE] = { .type = HVM_INST_LE, .name = "le", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_CMP] = { .type = HVM_INST_CMP, .name = "cmp", .has_operand = ut_false, .min_sp = 2, },

    [HVM_INST_FPUSH] = { .type = HVM_INST_FPUSH, .name = "fpush", .has_operand = ut_false, .min_sp = 0, },
    [HVM_INST_FADD] = { .type = HVM_INST_FADD, .name = "fadd", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_FSUB] = { .type = HVM_INST_FSUB, .name = "fsub", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_FMUL] = { .type = HVM_INST_FMUL, .name = "fmul", .has_operand = ut_false, .min_sp = 2, },
    [HVM_INST_FCMP] = { .type = HVM_INST_FCMP, .name = "fcmp", .has_operand = ut_false, .min_sp = 2, },

    [HVM_INST_JMP] = { .type = HVM_INST_JMP, .name = "jmp", .has_operand = ut_false, .min_sp = 0, },
    [HVM_INST_JZ] = { .type = HVM_INST_JZ, .name = "jz", .has_operand = ut_false, .min_sp = 1, },
    [HVM_INST_JN] = { .type = HVM_INST_JN, .name = "jn", .has_operand = ut_false, .min_sp = 1, },

};


HVM_Trap hvm_exec(HVM *vm, HVM_Inst inst)
{
#define HVM_X(vm) (vm)->stack[(vm)->ss + (vm)->sp - 1]
#define HVM_Y(vm) (vm)->stack[(vm)->ss + (vm)->sp - 2]
#define HVM_PUSH(VM, WORD) \
    do {                                                    \
        if((VM)->sp + 1 > HVM_STACK_CAPACITY)               \
            return HVM_TRAP_STACK_OVERFLOW;                 \
        (VM)->stack[(VM)->ss + (VM)->sp] = (WORD);          \
        (VM)->sp += 1;                                      \
    } while(0) 

    HVM_ASSERT(vm);
    HVM_InstInfo info = _inst_infos[inst.type];

    if(vm->sp < (uint32_t)info.min_sp) return HVM_TRAP_STACK_UNDERFLOW;
    vm->pc += 1;

    switch(inst.type) {
        case HVM_INST_HALT:
            {
                vm->halt = 1;
            } break;

        case HVM_INST_POP:
            {
                vm->sp -= 1;
            } break;

        case HVM_INST_COPY:
            {
                if((vm->ss + vm->sp) + 1 > HVM_STACK_CAPACITY) 
                    return HVM_TRAP_STACK_OVERFLOW;
                if(vm->sp < (uint32_t)inst.op.as_u64) 
                    return HVM_TRAP_STACK_UNDERFLOW;
                vm->stack[(vm->ss + vm->sp)] = vm->stack[(vm->ss + vm->sp) - 1 - inst.op.as_u64];
                vm->sp += 1;
            } break;

        case HVM_INST_BCOPY:
            {
                if((vm->ss + vm->sp) + 1 > HVM_STACK_CAPACITY) 
                    return HVM_TRAP_STACK_OVERFLOW;
                if(vm->sp < (uint32_t)inst.op.as_u64) 
                    return HVM_TRAP_STACK_UNDERFLOW;
                vm->stack[(vm->ss + vm->sp)] = vm->stack[vm->ss + inst.op.as_u64];
                vm->sp += 1;
            } break;

        case HVM_INST_SWAP:
            {
                if(vm->sp < (uint32_t)inst.op.as_u64) 
                    return HVM_TRAP_STACK_UNDERFLOW;
                HVM_Word tmp = vm->stack[(vm->ss + vm->sp) - 1 - inst.op.as_u64];
                vm->stack[(vm->ss + vm->sp) - 1 - inst.op.as_u64] = vm->stack[(vm->ss + vm->sp)];
                vm->stack[(vm->ss + vm->sp)] = tmp;
            } break;

        case HVM_INST_BSWAP:
            {
                if(vm->sp < (uint32_t)inst.op.as_u64) 
                    return HVM_TRAP_STACK_UNDERFLOW;
                HVM_Word tmp = vm->stack[vm->ss + inst.op.as_u64];
                vm->stack[vm->ss + inst.op.as_u64] = vm->stack[vm->ss + vm->sp - 1];
                vm->stack[vm->ss + vm->sp - 1] = tmp;
            } break;

        case HVM_INST_COPYABS:
            {
                if((vm->ss + vm->sp) + 1 > HVM_STACK_CAPACITY) 
                    return HVM_TRAP_STACK_OVERFLOW;
                if((vm->ss + vm->sp) < (uint32_t)inst.op.as_u64) 
                    return HVM_TRAP_STACK_UNDERFLOW;
                vm->stack[(vm->ss + vm->sp)] = vm->stack[inst.op.as_u64];
                vm->sp += 1;
            } break;

        case HVM_INST_SWAPABS:
            {
                if(vm->ss + vm->sp < (uint32_t)inst.op.as_u64) 
                    return HVM_TRAP_STACK_UNDERFLOW;
                HVM_Word tmp = vm->stack[inst.op.as_u64];
                vm->stack[inst.op.as_u64] = vm->stack[vm->ss + vm->sp - 1];
                vm->stack[vm->ss + vm->sp - 1] = tmp;
            } break;

        case HVM_INST_BEGIN_SCOPE:
            {
                HVM_PUSH(vm, HVM_WORD_U64(vm->ss));
                vm->ss = vm->ss + vm->sp;
                vm->sp = 0;
            } break;

        case HVM_INST_END_SCOPE:
            {
                uint32_t prev_ss = (uint32_t)vm->stack[vm->ss - 1].as_u64;
                vm->sp = vm->ss - 1;
                vm->ss = prev_ss;
            } break;

        case HVM_INST_PUSH:
            {
                HVM_PUSH(vm, inst.op);
            } break;

        case HVM_INST_JMP:
            {
                vm->pc = inst.op.as_u64;
            } break;
        case HVM_INST_JZ:
            {
                if(HVM_X(vm).as_i64 == 0) 
                    vm->pc = inst.op.as_u64;
                vm->sp -= 1;
            } break;
        case HVM_INST_JN:
            {
                if(HVM_X(vm).as_i64 != 0) 
                    vm->pc = inst.op.as_u64;
                vm->sp -= 1;
            } break;

        case HVM_INST_ADD:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 + HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        case HVM_INST_SUB:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 - HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        case HVM_INST_MUL:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 * HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        case HVM_INST_EQ:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 == HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        case HVM_INST_NE:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 != HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        case HVM_INST_GT:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 > HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        case HVM_INST_GE:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 >= HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        case HVM_INST_LT:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 < HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        case HVM_INST_LE:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 <= HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;

        case HVM_INST_CMP:
            {
                HVM_Y(vm).as_i64 = HVM_Y(vm).as_i64 - HVM_X(vm).as_i64;
                vm->sp -= 1;
            } break;
        default:
            return HVM_TRAP_INVALID_INSTRUCTION;
    }

    // TRACE_PRINTF("Executing %s(int(%ld)|float(%f)\n", info.name, 
    //     info.has_operand ? inst.op.as_i64 : 0,
    //     info.has_operand ? inst.op.as_f64 : 0.0);
    // hvm_dump(vm);

    return HVM_TRAP_NONE;
#undef HVM_X
#undef HVM_Y
#undef HVM_PUSH
}

void hvm_dump(const HVM *vm)
{
    HVM_ASSERT(vm);
    printf("VM (sp=%u, pc=%u)\n", vm->sp, vm->pc);
    for(uint32_t i = 0; i < vm->sp; ++i) {
        printf("    [0x%X] int(%ld)|float(%f)\n", i, vm->stack[i].as_i64, vm->stack[i].as_f64);
    }
}

void hvm_init(HVM *vm)
{
    HVM_ASSERT(vm);
    vm->pc = 0;
    vm->sp = 0;
    vm->ss = 0;
}

void hvm_module_init(HVM_Module *module)
{
    HVM_ASSERT(module);
    module->items = UT_NULL;
    module->count = 0;
    module->capacity = 0;
    buffer_init(&module->static_data);
}

void hvm_module_deinit(HVM_Module *module)
{
    HVM_FREE(module->items);
    module->count = 0;
    module->capacity = 0;
    module->items = UT_NULL;
}

void hvm_module_dump(const HVM_Module module)
{
    HVM_Inst inst;
    HVM_InstInfo info;
    for(uint32_t i = 0; i < module.count; ++i) {
        inst = module.items[i];
        info = _inst_infos[inst.type];
        printf("0x%X %s(int(%ld)|float(%f)\n", i, info.name, 
                info.has_operand ? inst.op.as_i64 : 0,
                info.has_operand ? inst.op.as_f64 : 0.0);
    }
}

void hvm_module_append(HVM_Module *module, HVM_Inst inst)
{
    HVM_ASSERT(module);
    if(module->count + 1 > module->capacity) {
        ut_size new_capacity = module->capacity * 2;
        if(new_capacity == 0) new_capacity = 32;

        HVM_Inst *new_items = HVM_MALLOC(sizeof(inst)*new_capacity);
        HVM_ASSERT(new_items);
        ut_memcpy(new_items, module->items, module->capacity * sizeof(inst));
        HVM_FREE(module->items);
        module->items = new_items;
        module->capacity = new_capacity;
    }

    module->items[module->count] = inst;
    module->count += 1;
}

HVM_Trap hvm_exec_module(HVM *vm, const HVM_Module module)
{
    HVM_ASSERT(vm);
    HVM_Inst inst;
    HVM_Trap res;
    while(!vm->halt) {
        inst = module.items[vm->pc];
        res = hvm_exec(vm, inst);
        if(res != HVM_TRAP_NONE) {
            HVM_InstInfo info = _inst_infos[inst.type];
            fprintf(stderr, "Trap %d is thrown while executing %s(ptr(%lu)|int(%ld)|float(%f)\n", res, info.name, 
                    info.has_operand ? inst.op.as_u64 : 0,
                    info.has_operand ? inst.op.as_i64 : 0,
                    info.has_operand ? inst.op.as_f64 : 0.0);
            hvm_dump(vm);
            break;
        }
    }
    return res;
}

ut_bool hvm_module_save_to_file(const HVM_Module module, const char *file_path)
{
    Arena a;
    HVM_ModuleFileHeader header;
    Buffer tmp;
    Buffer res;
    ut_memset(&a, 0, sizeof(a));

    header.version = HVM_VERSION;
    header.magic_number = HVM_MAGIC_NUMBER;
    header.insts_amount = module.count;
    header.program_size = sizeof(HVM_Inst) * module.count;

    tmp.count = 0;
    tmp.capacity = 0;

    for(ut_size i = 0; i < (ut_size)module.count; ++i) {
        HVM_Inst inst = module.items[i];
        buffer_append_with_arena(&tmp, (void *)&inst, sizeof(inst), &a);
    }
    header.program_start = 0;
    header.static_data_start = tmp.count;
    header.static_data_size = module.static_data.count;
    if(module.static_data.data) {
        buffer_append_with_arena(&tmp, module.static_data.data, module.static_data.count, &a);
    }

    res.count = 0;
    res.capacity = 0;
    res.data = 0;

    buffer_append_with_arena(&res, (void *)&header, sizeof(header), &a);
    buffer_append_with_arena(&res, tmp.data, tmp.count, &a);
    ut_bool retval = buffer_save_to_file(res, file_path);
    arena_free(&a);
    return retval;
}

ut_bool hvm_module_load_from_file(HVM_Module *module, const char *file_path, Arena *a)
{
    UT_ASSERT(module->count == 0 && module->capacity == 0);
    Buffer file;
    Arena local;
    ut_memset(&local, 0, sizeof(local));
    if(!buffer_load_from_file_with_arena(&file, file_path, &local)) {
        return ut_false;
    }

    HVM_ModuleFileHeader header;
    BufferView header_buffer = buffer_slice(file, 0, sizeof(HVM_ModuleFileHeader));
    ut_memcpy((void *)&header, header_buffer.data, header_buffer.count);

    TRACE_PRINTF("Loaded module with following information\n");
    TRACE_PRINTF("Magic Number: 0x%X\n", header.magic_number);
    TRACE_PRINTF("HVM Module Version\n");
    TRACE_PRINTF("    - major: %u\n", UT_VERSION_MAJOR(header.version));
    TRACE_PRINTF("    - minor: %u\n", UT_VERSION_MINOR(header.version));
    TRACE_PRINTF("    - revision: %u\n", UT_VERSION_REVISION(header.version));
    TRACE_PRINTF("Instruction amount: %u\n", header.insts_amount);
    TRACE_PRINTF("Program Size: %lu\n", header.program_size);
    TRACE_PRINTF("Program Start: %lu\n", header.program_start);
    TRACE_PRINTF("Static Data Size: %lu\n", header.static_data_size);
    TRACE_PRINTF("Static Data Start: %lu\n", header.static_data_start);

    BufferView insts_buffer = buffer_slice(file, header_buffer.count + header.program_start, header.program_start);
    BufferView static_data_buffer = buffer_slice(file, header_buffer.count + header.static_data_start, header.static_data_size);
    for(uint32_t i = 0; i < header.insts_amount; ++i) {
        HVM_Inst inst = ((HVM_Inst*)insts_buffer.data)[i];
        hvm_module_append(module, inst);
    }
    buffer_append_with_arena(&module->static_data, static_data_buffer.data, static_data_buffer.count, a);

    arena_free(&local);
    return ut_true;
}

