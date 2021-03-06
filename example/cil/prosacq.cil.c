/**
 * Copyright (C) 2017 Quenza Inc. All Rights Reserved.
 * Copyright (C) 2018 Blub Corp. All Rights Reserved.
 *
 * This file is part of the Cryptox project.
 *
 * Content can not be copied and/or distributed without the express
 * permission of the author.
 */

/// This file is autogenerated by the crygen code generator
/// and should not be altered in critical sections. Doing so may
/// result in untracable diagnostics.

/// Project   :       Cryptox Demo2
/// Name      :       Proposal Acquisition
/// Generated :       2017-09-24 11:58:43
/// Author    :       Blub Corp.
/// Language  :       C Intermediate Language
/// Dialect   :       CIL version 0.8
/// Standard  :       C99
/// O Level   :       0

// Compiler instructions
#pragma crycc auto 1
#pragma crycc optimize 0
#pragma crycc dialect cil08
#pragma crycc fallback cil02
#pragma crycc standard c99
#pragma crycc entry 1293
#pragma crycc init 9261
#pragma crycc finit 2741

// Project meta info
#pragma meta id 3004
#pragma meta timstamp 1506248672

#if !defined(_CRY_META_PROJECT)
# define _CRY_META_PROJECT   "Cryptox Demo2"
# define _CRY_META_NAME      "Proposal Acquisition"
# define _CRY_META_AUTHOR    "Blub Corp."
# define _CRY_META_ID        3004
# define _CRY_META_TIMESTAMP 1506248672
#endif

#ifdef __CRYC__
# include <crylib.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _CRY_WINDOWS
# pragma lib "smcbz.dll"
#elif defined(_CRY_LINUX)
# pragma lib "smcbz.so"
#endif

// Unit instructions
#pragma unit id 7112
#pragma unit order 4

#ifdef _CRY_UNIT_ID
# error "unit already defined"
#endif

#define _CRY_UNIT_ID    7112
#define _CRY_UNIT_ORDER 4

#pragma command (78263, "Print data as hexadecimal string") {
# define CRY_USER_DEFINED_OBJECT 1
# define CRY_PROCEDURE_ID 78263
# define CRY_PROCEDURE_DESC "Print data as hexadecimal string"
/* Print data as hexadecimal string */
void print_hex(unsigned char data[], size_t len) {
    printf("[ ");
    for (int i=0; i<len; ++i) {
        printf("%.2x", data[i]);
    }
    printf(" ]\n");
}
# undef CRY_USER_DEFINED_OBJECT
# undef CRY_PROCEDURE_DESC
# undef CRY_PROCEDURE_ID
#pragma } // 78263

#pragma member (83712, "keystream") {
# define CRY_MEMBER_ID 83712
# define CRY_MEMBER_DESC "keystream"
static const unsigned char keystream[] = {'a','#','$','*','['};
# undef CRY_MEMBER_ID
# undef CRY_MEMBER_DESC
#pragma } // 83712

#pragma procedure (78178, "XOR input data with modulo keystream") {
# define CRY_PROCEDURE_ID 78178
# define CRY_PROCEDURE_DESC "XOR input data with modulo keystream"
int xor(unsigned char data[], size_t len) {
    for (int i=0; i<len; ++i) {
        data[i] ^= keystream[i % sizeof(keystream)];
    }
}
# undef CRY_PROCEDURE_DESC
# undef CRY_PROCEDURE_ID
#pragma } // 78178

#pragma procedure (9261, "Application constructor") {
# define CRY_PROCEDURE_ID 9261
# define CRY_PROCEDURE_DESC "Application constructor"
void on_start() {
    printf("\t=< Project Report >=\n");
    printf("Project\t\t: %s\n", _CRY_META_PROJECT);
    printf("Project\t\t: %s\n", _CRY_META_PROJECT);
    printf("Name\t\t: %s\n", _CRY_META_NAME);
    printf("Author\t\t: %s\n", _CRY_META_AUTHOR);
    printf("\n");
}
# undef CRY_PROCEDURE_DESC
# undef CRY_PROCEDURE_ID
#pragma } // 9261

#pragma procedure (1293, "Application entry") {
# define CRY_PROCEDURE_ID 1293
# define CRY_PROCEDURE_DESC "Application entry"
int main(int argc, char *argv[]) {
    char str[] = "a testing string";
    printf("Original: %s ", str);
    print_hex(str, sizeof(str));

    xor(str, sizeof(str));
    printf("XOR:");
    print_hex(str, sizeof(str));

    xor(str, sizeof(str));
    printf("Original: %s ", str);
    print_hex(str, sizeof(str));

#if !defined(__CRYCC__)
    return 0;
#else
    return CRY_OK;
#endif
}
# undef CRY_PROCEDURE_DESC
# undef CRY_PROCEDURE_ID
#pragma } // 1293

#pragma procedure (2741, "Application destructor") {
# define CRY_PROCEDURE_ID 2741
# define CRY_PROCEDURE_DESC "Application destructor"
void on_stop() {
    //
}
# undef CRY_PROCEDURE_DESC
# undef CRY_PROCEDURE_ID
#pragma } // 2741

#undef _CRY_UNIT_ID
#undef _CRY_UNIT_ORDER

#pragma unit end
