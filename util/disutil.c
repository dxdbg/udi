/*
 * Copyright (c) 2011-2015, UDI Contributors
 * All rights reserved.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * A utility program used to experiment with libudis86
 */

#include <stdlib.h>
#include <stdio.h>

#include <udis86.h>

void func(int value) {
    if (value == 1) {
        printf("One\n");
    }else if ( value == 2 ) {
        printf("Two\n");
    }

    switch (value) {
        case 1:
            printf("One\n");
            break;
        case 2:
            printf("Two\n");
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]) {

    ud_t ud_obj;

    ud_init(&ud_obj);

    ud_set_input_buffer(&ud_obj, (unsigned char *)func, 100);

    ud_set_syntax(&ud_obj, UD_SYN_ATT);

    ud_set_mode(&ud_obj, 64);

    ud_set_pc(&ud_obj, (unsigned long)func);

    unsigned int pointer = 0, cur_insn_length = 0;
    while ( (cur_insn_length = ud_disassemble(&ud_obj)) != 0 ) {
        printf("func(%d): %s\n", pointer, ud_insn_asm(&ud_obj));
        pointer += cur_insn_length;
    }

    return EXIT_SUCCESS;
}
