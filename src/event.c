/**
 * event.c
 * craftos-base
 * 
 * This file defines the functions related to event handling.
 * 
 * This file is licensed under the MIT license.
 * Copyright (c) 2018-2026 JackMacWindows.
 */

#include <craftos.h>
#include "types.h"

/* TODO */

const char* ps2codes_sc1 = "\0\0321234567890-=\032\032qwertyuiop[]\032\032asdfghjkl;'`\032\\zxcvbnm,./\032\0\032 \032\032\032\032\032\032\032\032\032\032\032\032\032\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\032\032";
const char* ps2codes_sc1_upper = "\0\032!@#$%^&*()_+\032\032QWERTYUIOP{}\032\032ASDFGHJKL:\"~\032|ZXCVBNM<>?\032\0\032 \032\032\032\032\032\032\032\032\032\032\032\032\032\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\032\032";

void craftos_event_timer(craftos_machine_t machine, int id) {
    if (F.startTimer != NULL) {
        struct craftos_alarm_list ** alarm;
        for (alarm = &machine->alarms; *alarm; alarm = &(*alarm)->next) {
            if ((*alarm)->id == id) {
                struct craftos_alarm_list * a = *alarm;
                *alarm = a->next;
                F.free(a);
                craftos_machine_queue_event(machine, "alarm", "i", id);
                return;
            }
        }
    } else {
        /* TODO: software timers */
    }
    craftos_machine_queue_event(machine, "timer", "i", id);
}

void craftos_event_key(craftos_machine_t machine, unsigned char keycode, int repeat) {
    craftos_machine_queue_event(machine, "key", "iq", keycode, repeat);
    if (keycode == 42 || keycode == 54) machine->modifiers |= 1; /* shift key */
    else if (keycode < 91 && ps2codes_sc1[keycode] >= 0x20) craftos_machine_queue_event(machine, "char", "c", (machine->modifiers & 1 ? ps2codes_sc1_upper : ps2codes_sc1)[keycode]);
}

void craftos_event_key_up(craftos_machine_t machine, unsigned char keycode) {
    craftos_machine_queue_event(machine, "key_up", "i", keycode);
    if (keycode == 42 || keycode == 54) machine->modifiers &= ~1; /* shift key */
}

void craftos_event_mouse_click(craftos_machine_t machine, int button, unsigned int x, unsigned int y) {

}

void craftos_event_mouse_move(craftos_machine_t machine, unsigned int x, unsigned int y) {

}

void craftos_event_mouse_up(craftos_machine_t machine, int button, unsigned int x, unsigned int y) {

}

void craftos_event_mouse_scroll(craftos_machine_t machine, int direction, unsigned int x, unsigned int y) {

}
