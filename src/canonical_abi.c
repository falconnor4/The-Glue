#include "../include/canonical_abi.h"
#include <string.h> // For memcpy
#include <stdlib.h> // For NULL, malloc, free

// Helper function to get the ffi_type for a given CType.
static ffi_type* ctype_to_ffi_type(CType ctype) {
    switch (ctype) {
        case C_TYPE_VOID: return &ffi_type_void;
        case C_TYPE_CHAR: return &ffi_type_schar;
        case C_TYPE_SHORT: return &ffi_type_sshort;
        case C_TYPE_INT: return &ffi_type_sint;
        case C_TYPE_LONG: return &ffi_type_slong;
        case C_TYPE_FLOAT: return &ffi_type_float;
        case C_TYPE_DOUBLE: return &ffi_type_double;
        case C_TYPE_POINTER: return &ffi_type_pointer;
        case C_TYPE_STRUCT:
            // For the PoC, we pass all structs by pointer.
            // A real implementation would need to handle struct-by-value,
            // which is much more complex.
            return &ffi_type_pointer;
        default: return NULL;
    }
}

// Helper to get the size of a canonical type
static size_t get_canonical_type_size(CType ctype) {
    switch (ctype) {
        case C_TYPE_CHAR: return sizeof(char);
        case C_TYPE_SHORT: return sizeof(short);
        case C_TYPE_INT: return sizeof(int);
        case C_TYPE_LONG: return sizeof(long);
        case C_TYPE_FLOAT: return sizeof(float);
        case C_TYPE_DOUBLE: return sizeof(double);
        case C_TYPE_POINTER: return sizeof(void*);
        case C_TYPE_STRUCT: return sizeof(void*); // Passed by pointer
        default: return 0;
    }
}

size_t marshal_struct(const CanonicalStruct* cs, const void* native_struct_ptr, char* buffer, size_t buffer_size) {
    if (!cs || !native_struct_ptr || !buffer) {
        return 0;
    }

    if (buffer_size < cs->canonical_size) {
        return 0; // Buffer is too small
    }

    size_t canonical_offset = 0;
    for (int i = 0; i < cs->num_members; ++i) {
        const CanonicalMember* member = &cs->members[i];

        // Calculate the address of the member in the native struct
        const char* source_ptr = (const char*)native_struct_ptr + member->offset;

        // Copy the member's data into the canonical buffer
        memcpy(buffer + canonical_offset, source_ptr, member->size);

        // Advance the offset in the canonical buffer
        canonical_offset += member->size;
    }

    // The final canonical offset should match the pre-calculated canonical size
    if (canonical_offset != cs->canonical_size) {
        // This would indicate a mismatch in metadata, which is an error.
        return 0;
    }

    return canonical_offset;
}

int call_function(const CanonicalFunction* cf, void (*func_ptr)(), const char* args_buffer, CanonicalResult* result) {
    if (!cf || !func_ptr || !result) {
        return -1;
    }

    ffi_cif cif;
    ffi_type* ffi_return_type = ctype_to_ffi_type(cf->return_type);
    if (!ffi_return_type) return -1;

    // Prepare argument types for ffi_prep_cif
    ffi_type** ffi_arg_types = malloc(sizeof(ffi_type*) * cf->num_args);
    if (!ffi_arg_types) return -1;

    for (int i = 0; i < cf->num_args; ++i) {
        ffi_arg_types[i] = ctype_to_ffi_type(cf->arg_types[i]);
        if (!ffi_arg_types[i]) {
            free(ffi_arg_types);
            return -1;
        }
    }

    // Prepare the Call Interface (CIF)
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, cf->num_args, ffi_return_type, ffi_arg_types) != FFI_OK) {
        free(ffi_arg_types);
        return -1;
    }

    // Prepare the arguments themselves. libffi needs an array of pointers to the arguments.
    void** arg_pointers = malloc(sizeof(void*) * cf->num_args);
    if (!arg_pointers) {
        free(ffi_arg_types);
        return -1;
    }

    // We need to copy the packed arguments from our canonical buffer to a temporary
    // location because ffi needs correctly aligned pointers to each argument.
    size_t arg_values_size = 0;
    for(int i=0; i < cf->num_args; ++i) {
        arg_values_size += get_canonical_type_size(cf->arg_types[i]);
    }
    char* arg_values = malloc(arg_values_size);
    if (!arg_values) {
        free(ffi_arg_types);
        free(arg_pointers);
        return -1;
    }
    // Copy the packed buffer to our temporary correctly-aligned buffer
    memcpy(arg_values, args_buffer, arg_values_size);

    // Set up the pointers for ffi_call
    size_t current_offset = 0;
    for (int i = 0; i < cf->num_args; ++i) {
        arg_pointers[i] = arg_values + current_offset;
        current_offset += get_canonical_type_size(cf->arg_types[i]);
    }

    // The space for the return value
    void* rc_val = NULL;
    if (cf->return_type != C_TYPE_VOID) {
        rc_val = malloc(ffi_return_type->size);
        if (!rc_val) {
            free(ffi_arg_types);
            free(arg_pointers);
            free(arg_values);
            return -1;
        }
    }

    // Make the call
    ffi_call(&cif, func_ptr, rc_val, arg_pointers);

    // Store the result
    if (cf->return_type != C_TYPE_VOID) {
        switch (cf->return_type) {
            case C_TYPE_INT:
            case C_TYPE_SHORT:
            case C_TYPE_CHAR:
            case C_TYPE_LONG:
                result->long_val = *(long*)rc_val;
                break;
            case C_TYPE_FLOAT:
            case C_TYPE_DOUBLE:
                result->double_val = *(double*)rc_val;
                break;
            case C_TYPE_POINTER:
            case C_TYPE_STRUCT:
                result->ptr_val = *(void**)rc_val;
                break;
            default:
                break; // Should not happen
        }
    }

    // Cleanup
    free(ffi_arg_types);
    free(arg_pointers);
    free(arg_values);
    if (rc_val) free(rc_val);

    return 0;
}

size_t unmarshal_struct(const CanonicalStruct* cs, const char* buffer, size_t buffer_size, void* native_struct_ptr) {
    if (!cs || !buffer || !native_struct_ptr) {
        return 0;
    }

    // The buffer size must match the expected canonical size for a valid operation.
    if (buffer_size != cs->canonical_size) {
        return 0;
    }

    size_t canonical_offset = 0;
    for (int i = 0; i < cs->num_members; ++i) {
        const CanonicalMember* member = &cs->members[i];

        // Calculate the address of the member in the native struct
        char* dest_ptr = (char*)native_struct_ptr + member->offset;

        // Copy the member's data from the canonical buffer to the native struct
        memcpy(dest_ptr, buffer + canonical_offset, member->size);

        // Advance the offset in the canonical buffer
        canonical_offset += member->size;
    }

    if (canonical_offset != cs->canonical_size) {
        // Should not happen if the initial size check passes, but good for safety.
        return 0;
    }

    return canonical_offset;
}
