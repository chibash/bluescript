// Copyright (C) 2022- Shigeru Chiba.  All rights reserved.

#ifndef __C_RUNTIME_H__
#define __C_RUNTIME_H__

#include <stdint.h>
#include <stdbool.h>

/*
    All values are 32bit.  Its type is value_t.

    xxxx ... xxxx xx00          30 bit integer
    xxxx ... xxxx xx01          IEEE 754 binary32 but 21 bit (not 23 bit) fraction
    xxxx ... xxxx xx11          32 bit address (4 byte aligned)
    xxxx ... xxxx xx10          symbols (true and false)

    A 32bit address points to a structure of object_type type.
*/

#ifdef TEST64
#define ALGIN __attribute__ ((aligned(8)))
#define CR_SECTION 
#else
#define ALGIN __attribute__ ((aligned(4)))
#define CR_SECTION __attribute__((section(".text.cruntime")))
#endif

typedef uint32_t value_t;

struct func_body {
    void* fptr;
    const char* signature;
};

struct object_type {
    /*
      class_ptr:  30 bits (pointer to a class_object)
      is gray: 1 bit
      gc mark: 1 bit
    */
    uint32_t header;
    value_t body[1];
};

typedef struct object_type* pointer_t;

struct property_table {
    const uint16_t size;        // the number of properties declared in this class.
    const uint16_t offset;      // the index of the first property
    const uint16_t unboxed;     // 1 + the maximum index of the unboxed properties
    const uint16_t* const prop_names;   // property names
    const char* const unboxed_types; // the types of unboxed properties
};

struct method_table {
    const uint16_t size;        // the number of methods declared in this class.
    const uint16_t* const names;         // method names
    const char* const* const signatures; // method pointers
};

typedef struct class_object {
    int32_t size;           // instance size excluding a header.
                            // -1 if the instance is an array.
                            // Its size is stored in body[0] as int32_t.
    uint32_t start_index;   // the index for the first value_t elements.
                            // If start_index is 2, body[0] and body[1] don't hold a pointer.
    const char* const name; // printable class name
    const struct class_object* const superclass;    // super class or NULL
    const char* const array_type_name;  // encoded type name.  NULL if this type is not an array type.
                                        // An array type supports [] and .length.
    struct property_table table;
    struct method_table mtable;
    void* vtbl[1];          // virtual function table
} class_object;

// A macro for declaring a class_object.
// n: the length of body (> 0).
#define CLASS_OBJECT(name, n)    ALGIN const union { struct class_object clazz; struct { uint32_t s; uint32_t i; const char* const cn; const struct class_object* const sc; const char* const an; struct property_table pt; struct method_table mt; void* vtbl[n]; } body; } name

inline int32_t value_to_int(value_t v) { return (int32_t)v / 4; }
inline value_t int_to_value(int32_t v) { return (uint32_t)v << 2; }
inline bool is_int_value(value_t v) { return (v & 3) == 0; }

extern float CR_SECTION value_to_float(value_t v);
extern value_t CR_SECTION float_to_value(float v);
inline bool is_float_value(value_t v) { return (v & 3) == 1; }

#ifdef TEST64
extern pointer_t gc_heap_pointer(pointer_t ptr);
inline pointer_t value_to_ptr(value_t v) { return gc_heap_pointer((pointer_t)((uint64_t)v & 0xfffffffc)); }
#else
inline pointer_t value_to_ptr(value_t v) { return (pointer_t)(v & 0xfffffffc); }
#endif

inline value_t ptr_to_value(pointer_t v) { return (value_t)(((uintptr_t)v & 0xfffffffc) | 3); }
inline bool is_ptr_value(value_t v) { return (v & 3) == 3; }

#define VALUE_NULL    3         // null pointer: 0000 ... 0011
#define VALUE_UNDEF   3         // equivalent to VALUE_NULL
#define VALUE_ZERO    0         // integer 0
#define VALUE_FZERO   1         // float 0.0
#define VALUE_FALSE   2         // 0000 ... 0010
#define VALUE_TRUE    6         // 0000 ... 0110

extern bool CR_SECTION value_to_truefalse(value_t v);

inline value_t bool_to_value(bool b) { return b ? VALUE_TRUE : VALUE_FALSE; }
inline bool value_to_bool(value_t v) { return value_to_truefalse(v); }
inline bool is_bool_value(value_t v) { return (v & 3) == 2; }

inline bool safe_value_to_bool(value_t v) {
    // any value can be a boolean value.
    return value_to_bool(v);
}

struct gc_root_set {
    struct gc_root_set* next;
    uint32_t length;
    value_t values[1];
};

#define ROOT_SET_DECL(name,n)     struct { struct gc_root_set* next; uint32_t length; value_t values[n]; } name
#define ROOT_SET_INIT(name,n)     gc_init_rootset((struct gc_root_set*)&name, n);

#define DELETE_ROOT_SET(name)     { gc_root_set_head = name.next; }

#define ROOT_SET(name,n)    ROOT_SET_DECL(name,n); ROOT_SET_INIT(name,n)

#define VALUE_UNDEF_2       VALUE_UNDEF, VALUE_UNDEF
#define VALUE_UNDEF_3       VALUE_UNDEF, VALUE_UNDEF, VALUE_UNDEF
#define ROOT_SET_N(name,n,initv)     ROOT_SET_DECL(name,n) \
  = { .next = gc_root_set_head, .length = n, .values = { initv }}; gc_root_set_head = (struct gc_root_set*)&name;

extern int32_t CR_SECTION try_and_catch(void (*main_function)());
extern value_t CR_SECTION runtime_error(const char* msg);

extern int32_t CR_SECTION safe_value_to_int(value_t v);
extern float CR_SECTION safe_value_to_float(value_t v);
extern value_t CR_SECTION safe_value_to_null(value_t v);
extern value_t CR_SECTION safe_value_to_func(const char* signature, value_t func);
extern value_t CR_SECTION safe_value_to_string(bool nullable, value_t v);
extern value_t CR_SECTION safe_value_to_object(bool nullable, value_t v);
extern value_t CR_SECTION safe_value_to_value(bool nullable, const class_object* const clazz, value_t v);

extern value_t CR_SECTION any_add(value_t a, value_t b);
extern value_t CR_SECTION any_subtract(value_t a, value_t b);
extern value_t CR_SECTION any_multiply(value_t a, value_t b);
extern value_t CR_SECTION any_divide(value_t a, value_t b);
extern value_t CR_SECTION any_modulo(value_t a, value_t b);
extern value_t CR_SECTION any_power(value_t a, value_t b);
extern double CR_SECTION double_power(double a, double b);

extern bool CR_SECTION any_eq(value_t a, value_t b);
extern bool CR_SECTION any_less(value_t a, value_t b);
extern bool CR_SECTION any_less_eq(value_t a, value_t b);
extern bool CR_SECTION any_greater(value_t a, value_t b);
extern bool CR_SECTION any_greater_eq(value_t a, value_t b);

extern value_t CR_SECTION any_add_assign(value_t* a, value_t b);
extern value_t CR_SECTION any_subtract_assign(value_t* a, value_t b);
extern value_t CR_SECTION any_multiply_assign(value_t* a, value_t b);
extern value_t CR_SECTION any_divide_assign(value_t* a, value_t b);
extern value_t CR_SECTION any_add_member(value_t obj, int index, value_t v);
extern value_t CR_SECTION any_modulo_assign(value_t* a, value_t b);

extern value_t CR_SECTION any_increment(value_t* expr);
extern value_t CR_SECTION any_decrement(value_t* expr);
extern value_t CR_SECTION any_post_increment(value_t* expr);
extern value_t CR_SECTION any_post_decrement(value_t* expr);

extern value_t CR_SECTION minus_any_value(value_t v);

extern void CR_SECTION gc_write_barrier(pointer_t obj, value_t value);

// The following functions should be called at start or end of interrupt handler.
extern void CR_SECTION interrupt_handler_start();
extern void CR_SECTION interrupt_handler_end();

extern void CR_SECTION gc_initialize();
extern class_object* CR_SECTION gc_get_class_of(value_t value);
extern bool CR_SECTION gc_is_instance_of(const class_object* clazz, value_t obj);
extern void* CR_SECTION gc_method_lookup(value_t obj, uint32_t index);
extern value_t CR_SECTION gc_dynamic_method_call(value_t obj, uint32_t index, uint32_t num, ...);

extern pointer_t CR_SECTION gc_allocate_object(const class_object* clazz);

// allocate a new instance of the given class.
inline value_t gc_new_object(const class_object* clazz) {
    return ptr_to_value(gc_allocate_object(clazz));
}

inline value_t get_obj_property(value_t obj, int index) {
    return value_to_ptr(obj)->body[index];
}

extern value_t* CR_SECTION get_obj_property_addr(value_t obj, int index);

inline value_t set_obj_property(value_t obj, int index, value_t new_value) {
    pointer_t objp = value_to_ptr(obj);
    gc_write_barrier(objp, new_value);
    return objp->body[index] = new_value;
}

inline int32_t* get_obj_int_property(value_t obj, int index) {
    return (int32_t*)&value_to_ptr(obj)->body[index];
}

inline float* get_obj_float_property(value_t obj, int index) {
    return (float*)&value_to_ptr(obj)->body[index];
}

inline value_t set_global_variable(value_t* ptr, value_t new_value) {
    gc_write_barrier((pointer_t)0, new_value);
    return *ptr = new_value;
}

extern value_t CR_SECTION get_anyobj_property(value_t obj, int property);
extern value_t CR_SECTION get_anyobj_legth_property(value_t obj, int property);
extern value_t CR_SECTION set_anyobj_property(value_t obj, int property, value_t new_value);
extern value_t CR_SECTION acc_anyobj_property(value_t obj, char op, int property, value_t new_value);

extern value_t CR_SECTION gc_new_function(void* fptr, const char* signature, value_t this_object);
extern bool CR_SECTION gc_is_function_object(value_t obj, const char* signature);
extern const void* CR_SECTION gc_function_object_ptr(value_t obj, int index);
extern value_t CR_SECTION gc_function_captured_value(value_t obj, int index);

extern value_t CR_SECTION gc_new_box(value_t value);
extern value_t CR_SECTION gc_new_int_box(int32_t value);
extern value_t CR_SECTION gc_new_float_box(float value);

extern value_t CR_SECTION gc_new_string(char* str);
extern bool CR_SECTION gc_is_string_object(value_t obj);
extern const char* CR_SECTION gc_string_to_cstr(value_t obj);
extern int32_t CR_SECTION gc_string_length(value_t obj);
extern char* CR_SECTION gc_any_to_cstring(char* p, value_t obj);
extern value_t CR_SECTION gc_new_String(value_t s1, value_t s2);

extern value_t CR_SECTION safe_value_to_intarray(bool nullable, value_t v);
extern value_t CR_SECTION gc_new_intarray(int32_t n, int32_t init_value);
extern value_t CR_SECTION gc_make_intarray(int32_t n, ...);
extern int32_t CR_SECTION gc_intarray_length(value_t obj);
extern int32_t* CR_SECTION gc_intarray_get(value_t obj, int32_t index);
extern bool CR_SECTION gc_is_intarray(value_t v);

extern value_t CR_SECTION safe_value_to_floatarray(bool nullable, value_t v);
extern value_t CR_SECTION gc_new_floatarray(int32_t n, float init_value);
extern value_t CR_SECTION gc_make_floatarray(int32_t n, ...);
extern int32_t CR_SECTION gc_floatarray_length(value_t obj);
extern float* CR_SECTION gc_floatarray_get(value_t obj, int32_t index);
extern bool CR_SECTION gc_is_floatarray(value_t v);

extern value_t CR_SECTION safe_value_to_boolarray(bool nullable, value_t v);
extern value_t CR_SECTION gc_new_bytearray(bool is_boolean, int32_t n, int32_t init_value);
extern value_t CR_SECTION gc_make_bytearray(bool is_boolean, int32_t n, ...);
extern int32_t CR_SECTION gc_bytearray_length(value_t obj);
extern uint8_t* CR_SECTION gc_bytearray_get(value_t obj, int32_t index);
extern bool CR_SECTION gc_is_boolarray(value_t v);

extern value_t CR_SECTION safe_value_to_vector(bool nullable, value_t v);
extern value_t CR_SECTION gc_new_vector(int32_t n, value_t init_value);
extern int32_t CR_SECTION gc_vector_length(value_t obj); 
extern value_t CR_SECTION gc_vector_get(value_t obj, int32_t index);
extern value_t CR_SECTION gc_vector_set(value_t obj, int32_t index, value_t new_value);
extern value_t CR_SECTION gc_make_vector(int32_t n, ...);

extern bool CR_SECTION gc_is_instance_of_array(value_t obj);
extern value_t CR_SECTION safe_value_to_anyarray(bool nullable, value_t v);
extern value_t CR_SECTION gc_new_array(const class_object* clazz, int32_t n, value_t init_value);
extern value_t CR_SECTION gc_make_array(const class_object* clazz, int32_t n, ...);
extern int32_t CR_SECTION gc_array_length(value_t obj);
extern value_t* CR_SECTION gc_array_get(value_t obj, int32_t index);
extern value_t CR_SECTION gc_array_set(value_t obj, int32_t index, value_t new_value);

extern int32_t gc_array_push(value_t obj, value_t new_value);
extern value_t gc_array_pop(value_t obj);
extern int32_t gc_array_unshift(value_t obj, value_t new_value);
extern value_t gc_array_shift(value_t obj);

extern int32_t CR_SECTION get_all_array_length(value_t obj);
extern value_t CR_SECTION get_anyobj_length_property(value_t obj, int property);
extern value_t CR_SECTION gc_safe_array_get(value_t obj, int32_t idx);
extern value_t CR_SECTION gc_safe_array_set(value_t obj, int32_t idx, value_t new_value);
extern value_t CR_SECTION gc_safe_array_acc(value_t obj, int32_t index, char op, value_t new_value);
extern bool CR_SECTION gc_is_anyarray(value_t v);

extern void CR_SECTION gc_init_rootset(struct gc_root_set* set, uint32_t length);
extern void CR_SECTION gc_run();

extern struct gc_root_set* gc_root_set_head;

#endif
