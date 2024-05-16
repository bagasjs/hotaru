/*

   `utils.h` - My own simple utility library (you can say it's my own standard library for working with C)

*/

#ifndef UTILS_H_
#define UTILS_H_

#define UTILS_VERSION UT_MAKE_VERSION(0, 1, 0)

#define ARENA_REGION_BACKEND_LINUX_MMAP 1
#define ARENA_REGION_BACKEND_VIRTUAL_ALLOC 2

#if defined(_WIN32)
    #define UT_TARGET_WIN32
#elif defined(__linux__)
    #ifdef __ANDROID__
        #define UT_TARGET_ANDROID
        #error "Currently not supporting the Android platform"
    #else
        #define UT_TARGET_LINUX
    #endif
#else
    #error "Unsupported platform"
#endif

#ifndef UTDEF
#define UTDEF
#endif

#ifndef UT_UNUSED
#define UT_UNUSED(x) (void)(x)
#endif

#ifndef UT_NULLABLE
#define UT_NULLABLE
#endif

#ifndef UT_ARRAY_LEN
#define UT_ARRAY_LEN(xs) (sizeof(xs)/sizeof(*xs))
#endif

#ifndef UT_SWAP
#define UT_SWAP(T, a, b) \
    do { \
        T tmp = (a); \
        (a) = (b); \
        (b) = tmp; \
    } while(0)
#endif

#ifndef UT_MAKE_VERSION
#define UT_MAKE_VERSION(major, minor, rev) ((ut_uint32)(((major) << 22) | ((minor) << 12) | (rev)))
#endif

#ifndef UT_VERSION_MAJOR
#define UT_VERSION_MAJOR(version) ((ut_uint32)((version) >> 22) & 0x3FF)
#endif

#ifndef UT_VERSION_MINOR
#define UT_VERSION_MINOR(version) ((ut_uint32)((version) >> 12) & 0x3FF)
#endif

#ifndef UT_VERSION_REVISION
#define UT_VERSION_REVISION(version) ((ut_uint32)((version) & 0xFFF))
#endif

#ifndef UT_STATIC_ASSERT
#define UT_STATIC_ASSERT(cond) _Static_assert(cond, #cond)
#endif

#ifndef UT_LITERAL
    #ifdef __cplusplus
        #define UT_LITERAL(T) T
    #else
        #define UT_LITERAL(T) (T)
    #endif
#endif

#ifndef UT_ASSERT
#include <assert.h>
#define UT_ASSERT(cond) assert(cond)
#endif

#ifndef UT_NULL
#define UT_NULL ((void *)0)
#endif

#ifndef ut_false
#define ut_false 0
#endif

#ifndef ut_true
#define ut_true 1
#endif

typedef unsigned char ut_uint8;
UT_STATIC_ASSERT(sizeof(ut_uint8) == 1);

typedef unsigned int  ut_uint32;
UT_STATIC_ASSERT(sizeof(ut_uint32) == 4);

typedef ut_uint8 ut_bool;

typedef unsigned long long ut_size;

typedef void *ut_uintptr;

#ifndef ARENA_REGION_BACKEND
    #if defined(UT_TARGET_LINUX)
        #define ARENA_REGION_BACKEND ARENA_REGION_BACKEND_LINUX_MMAP
    #elif defined(UT_TARGET_WIN32)
        #define ARENA_REGION_BACKEND ARENA_REGION_BACKEND_VIRTUAL_ALLOC
        #error "Not implemented"
    #else
        #error "Not supported platform"
    #endif
#endif

#ifndef DEFAULT_ARENA_REGION_SIZE
#define DEFAULT_ARENA_REGION_SIZE (32*1024)
#endif

typedef struct Buffer {
    void *data;
    ut_size count;
    ut_size capacity;
} Buffer;

typedef struct BufferView {
    const void *data;
    ut_size count;
} BufferView;

typedef struct ArenaRegion ArenaRegion;
struct ArenaRegion {
    ArenaRegion *next;
    ut_size count;
    ut_size capacity;

    ut_uintptr data[];
};

typedef struct Arena {
    ArenaRegion *begin;
    ArenaRegion *end;
} Arena;

typedef struct StringView {
    const char *data;
    ut_size count;
} StringView;

#define arena_da_append(a, da, item) \
    do { \
        if((da)->count + 1 > (da)->capacity) { \
            ut_size per_item_size = sizeof(*(da)->items); \
            ut_size new_capacity = (da)->capacity * 2; \
            if(new_capacity == 0) new_capacity = 8; \
            void *new_items = arena_malloc((a), new_capacity * per_item_size); \
            UT_ASSERT(new_items && "Buy more RAM LOL!"); \
            if((da)->items) ut_memcpy(new_items, (da)->items, \
                    (da)->count * per_item_size); \
            (da)->items = new_items; \
            (da)->capacity = new_capacity; \
        } \
        (da)->items[(da)->count++] = (item); \
    } while(0)

#define arena_da_append_many(a, da, new_items, new_items_count) \
    do { \
        ut_size per_item_size = sizeof(*(da)->items); \
        if((da)->count + (new_items_count) > (da)->capacity) { \
            ut_size new_capacity = (da)->capacity * 2 + (new_items_count); \
            if(new_capacity == 0) new_capacity = (new_items_count) + 8; \
            void *new_items2 = arena_malloc((a), new_capacity * per_item_size); \
            UT_ASSERT(new_items2 && "Buy more RAM LOL!"); \
            if((da)->items) ut_memcpy(new_items2, (da)->items, \
                    (da)->count * per_item_size); \
            (da)->items = new_items2; \
            (da)->capacity = new_capacity; \
        } \
        ut_memcpy((da)->items + (da)->count, (new_items), (new_items_count)*per_item_size); \
        (da)->count += (new_items_count); \
    } while(0)

#define SV_STATIC(s) { .data = (s), .count = ((sizeof(s) - 1)/sizeof(char))  }
#define SV(s) (StringView){ .data = (s), .count = ((sizeof(s) - 1)/sizeof(char)) }
#define SV_FMT "%.*s"
#define SV_ARGV(sv) (int)(sv).count, (sv).data

UTDEF StringView sv_from(const void *data, ut_size count);
UTDEF StringView sv_from_cstr(const char *cstr);
UTDEF StringView sv_slice(StringView view, ut_size start, ut_size end);
UTDEF ut_bool sv_eq(StringView a, StringView b);
UTDEF ut_bool sv_has_prefix(StringView sv, StringView prefix);
UTDEF ut_bool sv_has_suffix(StringView sv, StringView suffix);
UTDEF int sv_find(StringView sv, StringView needle, ut_size index);
UTDEF int sv_to_int(StringView view);

UTDEF ArenaRegion *create_arena_region(ut_size capacity);
UTDEF void destroy_arena_region(ArenaRegion *region);

UTDEF void *arena_malloc(Arena *a, ut_size size);
UTDEF void  arena_free(Arena *a);
UTDEF void  arena_reset(Arena *a);

UTDEF ut_bool ut_isalpha(char ch);
UTDEF ut_bool ut_isspace(char ch);
UTDEF ut_bool ut_isalnum(char ch);
UTDEF ut_bool ut_isdigit(char ch);
UTDEF ut_size ut_strlen(const char *cstr);
UTDEF void *ut_memcpy(void *dst, const void *src, ut_size size);
UTDEF void *ut_memset(void *dst, const int val, ut_size size);

UTDEF void buffer_init(Buffer *buf);
UTDEF void buffer_append_with_arena(Buffer *buf, const void *data, ut_size datasz, Arena *a);
UTDEF BufferView buffer_slice(Buffer buf, ut_size start, ut_size size);

#ifndef UTILS_WITHOUT_STDIO
UTDEF ut_bool buffer_save_to_file(const Buffer buf, const char *file_path);
UTDEF ut_bool buffer_load_from_file_with_arena(Buffer *buf, const char *file_path, Arena *a);
UTDEF char *load_file_text_with_arena(const char *file_path, Arena *a);
#endif

#endif // UTILS_H_

#ifdef UTILS_IMPLEMENTATION

#if ARENA_REGION_BACKEND == ARENA_REGION_BACKEND_LINUX_MMAP
#include <sys/mman.h>

ArenaRegion *create_arena_region(ut_size capacity)
{
    ut_size byte_size = sizeof(ArenaRegion) + (capacity * sizeof(ut_uintptr));
    ArenaRegion *r = mmap(UT_NULL, byte_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(!r) {
        return UT_NULL;
    }
    r->capacity = capacity;
    r->count = 0;
    r->next = UT_NULL;
    return r;
}

void destroy_arena_region(ArenaRegion *region)
{
    ut_size byte_size = sizeof(ArenaRegion) + (region->capacity * sizeof(ut_uintptr));
    munmap(region, byte_size);
}

#endif

void *arena_malloc(Arena *a, ut_size size_bytes)
{
    ut_size size = (size_bytes + sizeof(ut_uintptr) - 1)/sizeof(ut_uintptr);
    if(a->end == UT_NULL) {
        UT_ASSERT(a->begin == UT_NULL);
        ut_size capacity = DEFAULT_ARENA_REGION_SIZE;
        if(capacity < size) capacity = size;
        a->end = create_arena_region(capacity);
        a->begin = a->end;
    }

    while((a->end->count + size > a->end->capacity) && (a->end->next != UT_NULL)) {
        a->end = a->end->next;
    }

    if(a->end->count + size > a->end->capacity) {
        UT_ASSERT(a->end->next == UT_NULL);
        size_t capacity = DEFAULT_ARENA_REGION_SIZE;
        if(capacity < size) capacity = size;
        a->end->next = create_arena_region(capacity);
        a->end = a->end->next;
    }

    void *result = &a->end->data[a->end->count];
    a->end->count += size;
    return result;
}

void arena_free(Arena *a)
{
    ArenaRegion *r = a->begin;
    while (r) {
        ArenaRegion *r0 = r;
        r = r->next;
        destroy_arena_region(r0);
    }
    a->begin = UT_NULL;
    a->end = UT_NULL;
}

void arena_reset(Arena *a)
{
    for (ArenaRegion *r = a->begin; r != UT_NULL; r = r->next) {
        r->count = 0;
    }

    a->end = a->begin;
}

void *ut_memcpy(void *dst, const void *src, ut_size size)
{
    for(ut_size i = 0; i < size; ++i) {
        ((ut_uint8 *)dst)[i] = ((const ut_uint8 *)src)[i];
    }
    return dst;
}

void *ut_memset(void *dst, const int val, ut_size size)
{
    for(ut_size i = 0; i < size; ++i) {
        ((ut_uint8 *)dst)[i] = ((const ut_uint8)val);
    }
    return dst;
}

ut_bool ut_isspace(char ch)
{
    return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
}

ut_bool ut_isalpha(char ch)
{
    return ('A' <= ch && ch <= 'Z') 
        || ('a' <= ch && ch <= 'z');
}

ut_bool ut_isalnum(char ch)
{
    return ('A' <= ch && ch <= 'Z') 
        || ('a' <= ch && ch <= 'z')
        || ('0' <= ch && ch <= '9')
        ;
}

ut_bool ut_isdigit(char ch)
{
    return ('0' <= ch && ch <= '9');
}

ut_size ut_strlen(const char *cstr)
{
    ut_size i;
    for(i = 0; cstr[i] != '\0'; ++i);
    return i;
}

StringView sv_from(const void *data, ut_size count)
{
    StringView result;
    result.data = data;
    result.count = count;
    return result;
}

StringView sv_from_cstr(const char *cstr)
{
    StringView result;
    result.data = cstr;
    result.count = ut_strlen(cstr);
    return result;
}

StringView sv_slice(StringView view, ut_size start, ut_size end)
{
    if(end < start) UT_SWAP(size_t, start, end);

    StringView res;
    res.data = 0;
    res.count = 0;
    if(view.count < start) 
        return res;

    res.data = view.data + start;
    res.count = end - start;
    return res;
}

int sv_to_int(StringView view)
{
    ut_bool is_negative = ut_false;
    if(view.data[0] == '-') {
        is_negative = ut_true;
        view.count -= 1;
        view.data += 1;
    }
    int result = 0;
    for (size_t i = 0; i < view.count && ut_isdigit(view.data[i]); ++i) {
        result = result * 10 + (int) view.data[i] - '0';
    }
    if(is_negative) result *= -1;
    return result;
}

ut_bool sv_eq(StringView a, StringView b)
{
    if(a.count != b.count) return ut_false;
    for(ut_size i = 0; i < a.count; ++i) {
        if(a.data[i] != b.data[i]) return ut_false;
    }
    return ut_true;
}

ut_bool sv_has_prefix(StringView sv, StringView prefix)
{
    if(sv.count < prefix.count) return ut_false;

    for(ut_size i = 0; i < prefix.count; ++i) {
        if(sv.data[i] != prefix.data[i]) 
            return ut_false;
    }

    return ut_true;
}

ut_bool sv_has_suffix(StringView sv, StringView suffix)
{
    if(sv.count < suffix.count) return ut_false;

    for(ut_size i = 0; i < suffix.count; ++i) {
        if(sv.data[sv.count - suffix.count + i] != suffix.data[i])
            return ut_false;
    }

    return ut_true;
}

int sv_find(StringView sv, StringView needle, ut_size index)
{
    if(sv.count < needle.count) return -1;

    ut_size j = 0;
    ut_size found = 0;
    for(ut_size i = 0; i < sv.count; ++i) {
        if(sv.data[i] == needle.data[j]) {
            j += 1;
            if(j >= needle.count) {
                if(found == index) return i - j + 1;
                found += 1;
            }
        } else if(j > 0) {
            j = 0;
        }
    }

    return -1;
}

void buffer_init(Buffer *buf)
{
    buf->data = 0;
    buf->count = 0;
    buf->capacity = 0;
}

void buffer_append_with_arena(Buffer *buf, const void *data, ut_size datasz, Arena *a)
{
    UT_ASSERT(buf);
    UT_ASSERT(data);
    UT_ASSERT(a);

    if(buf->count + datasz > buf->capacity) {
        ut_size new_capacity = buf->capacity * 2 + datasz;
        if(new_capacity == 0) new_capacity = 32 + datasz;

        void *new_data = arena_malloc(a, new_capacity);
        UT_ASSERT(new_data);
        ut_memcpy(new_data, buf->data, buf->capacity);
        buf->data = new_data; 
        buf->capacity = new_capacity;
    }

    ut_memcpy((ut_uint8 *)buf->data + buf->count, data, datasz);
    buf->count += datasz;
}

BufferView buffer_slice(Buffer buf, ut_size start, ut_size size)
{
    BufferView result;
    result.data = (ut_uint8 *)buf.data + start;
    result.count = size;
    return result;
}

#ifndef UTILS_WITHOUT_STDIO
#include <stdio.h>
ut_bool buffer_save_to_file(const Buffer buf, const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if(!f) return ut_false;
    fwrite(buf.data, buf.count, 1, f);
    fclose(f);
    return ut_true;
}

ut_bool buffer_load_from_file_with_arena(Buffer *buf, const char *file_path, Arena *a)
{
    FILE *f = fopen(file_path, "rb");
    if(!f) return ut_false;
    ut_size fsz;
    fseek(f, 0, SEEK_END);
    fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *data = arena_malloc(a, fsz);
    if(!data) {
        fclose(f);
        return ut_false;
    }

    ut_size rsz = fread(data, 1, fsz, f);
    if(rsz != fsz) {
        fclose(f);
        return ut_false;
    }

    buf->data = data;
    buf->count = fsz;
    buf->capacity = fsz;
    return ut_true;
}

char *load_file_text_with_arena(const char *file_path, Arena *a)
{
    FILE *f = fopen(file_path, "r");
    if(!f) return UT_NULL;
    ut_size fsz;
    fseek(f, 0, SEEK_END);
    fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *data = arena_malloc(a, fsz);
    if(!data) {
        fclose(f);
        return UT_NULL;
    }

    ut_size rsz = fread(data, 1, fsz, f);
    if(rsz != fsz) {
        fclose(f);
        return UT_NULL;
    }

    return data;
}

#endif

#endif // UTILS_IMPLEMENTATION
