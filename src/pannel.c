/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  Xiegu X6100 LVGL GUI
 *
 *  Copyright (c) 2022-2023 Belousov Oleg aka R1CBU
 */

#include <stdio.h>
#include <stdlib.h>
#include "pannel.h"
#include "styles.h"
#include "util.h"
#include "scheduler.h"
#include "radio.h"
#include "params/params.h"
#include "rtty.h"

static lv_obj_t     *obj;
static char         buf[1024];
static char         tmp_buf[1024];
static char         *last_line;

static void check_lines() {
    char        *second_line = NULL;
    char        *ptr = (char *) &buf;
    uint16_t    count = 0;

    while (*ptr) {
        if (*ptr == '\n') {
            count++;

            if (count == 1) {
                second_line = ptr + 1;
            }
        }
        ptr++;
    }

    if (count > 4) {
        strcpy(tmp_buf, second_line);
        strcpy(buf, tmp_buf);
    }

    ptr = (char *) &buf;

    while (*ptr) {
        if (*ptr == '\n') {
            last_line = ptr + 1;
        }
        ptr++;
    }

    *last_line = '\0';
}

static void pannel_update_cb(const char *text) {
    lv_point_t line_size;
    lv_point_t text_size;

    if (!last_line) {
        return;
    }

    if (strcmp(text, "\n") == 0) {
        if (last_line[strlen(last_line) - 1] != '\n') {
            strcat(last_line, text);
            check_lines();
        }
    } else {
        lv_txt_get_size(&line_size, last_line, &sony_38, 0, 0, LV_COORD_MAX, 0);
        lv_txt_get_size(&text_size, text, &sony_38, 0, 0, LV_COORD_MAX, 0);

        if (line_size.x + text_size.x > (lv_obj_get_width(obj) - 40)) {
            strcat(last_line, "\n");
            check_lines();
        }

        strcat(last_line, text);
    }

    lv_label_set_text_static(obj, buf);
}

lv_obj_t * pannel_init(lv_obj_t *parent) {
    obj = lv_label_create(parent);

    lv_obj_add_style(obj, &pannel_style, 0);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

    return obj;
}

void pannel_add_text(const char * text) {
    scheduler_put(pannel_update_cb, text, strlen(text) + 1);
}

void pannel_hide() {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void pannel_visible() {
    x6100_mode_t    mode = radio_current_mode();
    bool            on = false;

    switch (mode) {
        case x6100_mode_cw:
        case x6100_mode_cwr:
            on = params.cw_decoder;
            break;

        case x6100_mode_usb:
        case x6100_mode_lsb:
        case x6100_mode_usb_dig:
        case x6100_mode_lsb_dig:
            on = rtty_get_state() != RTTY_OFF;
            break;
    }

    if (on) {
        strcpy(buf, "");
        last_line = (char *) &buf;
        lv_label_set_text_static(obj, buf);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}
