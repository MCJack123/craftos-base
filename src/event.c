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
}

void craftos_event_key_up(craftos_machine_t machine, unsigned char keycode) {
    craftos_machine_queue_event(machine, "key_up", "i", keycode);
}

void craftos_event_mouse_click(craftos_machine_t machine, int button, unsigned int x, unsigned int y) {

}

void craftos_event_mouse_move(craftos_machine_t machine, unsigned int x, unsigned int y) {

}

void craftos_event_mouse_up(craftos_machine_t machine, int button, unsigned int x, unsigned int y) {

}

void craftos_event_mouse_scroll(craftos_machine_t machine, int direction, unsigned int x, unsigned int y) {

}
