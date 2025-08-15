#include <stdio.h>
#include "../include/canonical_abi.h"
#include "simple_header.h"

/*
This file is a manual implementation of the code that the shim_generator tool
was intended to produce. It defines the metadata and wrapper functions for the
functions declared in simple_header.h.
*/

// --- METADATA and WRAPPER for add ---
const CType add_arg_types[] = { C_TYPE_INT, C_TYPE_INT };
const CanonicalFunction add_meta = {
    .name = "add",
    .return_type = C_TYPE_INT,
    .num_args = 2,
    .arg_types = add_arg_types
};

CanonicalResult canonical_wrapper_add(const char* args_buffer) {
    CanonicalResult result;
    // The function pointer &add is resolved by the C compiler/linker.
    call_function(&add_meta, (void (*)())&add, args_buffer, &result);
    return result;
}


// --- METADATA and WRAPPER for print_message ---
const CType print_message_arg_types[] = { C_TYPE_POINTER };
const CanonicalFunction print_message_meta = {
    .name = "print_message",
    .return_type = C_TYPE_VOID,
    .num_args = 1,
    .arg_types = print_message_arg_types
};

CanonicalResult canonical_wrapper_print_message(const char* args_buffer) {
    CanonicalResult result;
    call_function(&print_message_meta, (void (*)())&print_message, args_buffer, &result);
    return result;
}


// --- METADATA and WRAPPER for average ---
const CType average_arg_types[] = { C_TYPE_DOUBLE, C_TYPE_DOUBLE };
const CanonicalFunction average_meta = {
    .name = "average",
    .return_type = C_TYPE_DOUBLE,
    .num_args = 2,
    .arg_types = average_arg_types
};

CanonicalResult canonical_wrapper_average(const char* args_buffer) {
    CanonicalResult result;
    call_function(&average_meta, (void (*)())&average, args_buffer, &result);
    return result;
}
