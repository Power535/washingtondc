/*******************************************************************************
 *
 *
 *    WashingtonDC Dreamcast Emulator
 *    Copyright (C) 2019 snickerbockers
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 ******************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>

#include "log.h"
#include "washdc/fifo.h"
#include "hostfile.h"

#include "washdc/config_file.h"

#define CFG_NODE_KEY_LEN 256
#define CFG_NODE_VAL_LEN 256

struct cfg_node {
    struct fifo_node list_node;
    char key[CFG_NODE_KEY_LEN];
    char val[CFG_NODE_VAL_LEN];
};

enum cfg_parse_state {
    CFG_PARSE_PRE_KEY,
    CFG_PARSE_KEY,
    CFG_PARSE_PRE_VAL,
    CFG_PARSE_VAL,
    CFG_PARSE_POST_VAL,
    CFG_PARSE_ERROR
};

static struct cfg_state {
    enum cfg_parse_state state;
    unsigned key_len, val_len;
    char key[CFG_NODE_KEY_LEN];
    char val[CFG_NODE_VAL_LEN];
    unsigned line_count;
    struct fifo_head cfg_nodes;
    bool in_comment;
} cfg_state;

static void cfg_add_entry(void);
static void cfg_handle_newline(void);
static int cfg_parse_bool(char const *val, bool *outp);

static void cfg_create_default_config(void) {
    char const *cfg_file_path = hostfile_cfg_file();
    char const *cfg_file_dir = hostfile_cfg_dir();

    static char const *cfg_default =
        ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;"
        ";;;;;;;;;;\n"
        ";;\n"
        ";; AUTOMATICALLY GENERATED BY WASHINGTONDC\n"
        ";;\n"
        ";; This is WashingtonDC's config file.  Config settings consist of a "
        "config\n"
        ";; name followed by its value on the same line\n"
        ";; the semicolon (;) character can be used to create single-line "
        "comments\n"
        ";;\n"
        ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;"
        ";;;;;;;;;;\n"
        "\n"
        "; background color (use html hex syntax)\n"
        "ui.bgcolor #3d77c0\n"
        "\n"
        "; vsync\n"
        "; options are true (to enable) or false (to disable)\n"
        "win.vsync false\n"
        "\n"
        "; default external resolution.  this is the size of the window when\n"
        "; it is initially created, but you can also change it at runtime by\n"
        "; resizing the window.\n"
        "win.external-res.x 640\n"
        "win.external-res.y 480\n"
        "\n"
        "; Pick what kind of window you want.\n"
        "; available values are:\n"
        "; * windowed\n"
        "; * fullscreen\n"
        "win.window-mode windowed\n"
        "\n"
        "; how to filter the final image.  choices are:\n"
        ";     nearest - for nearest-neighbor sampling\n"
        ";     linear - for bilinear interpolation\n"
        "gfx.output.filter linear\n"
        "\n"
        "; Order-Independent Transparency algorithm.  choices are:\n"
        ";     disabled - no order-independent transparency\n"
        ";     per-group - groups of transparent polygons are sorted by depth\n"
        "; Ideally there would be a per-pixel mode, as well, but that hasn't\n"
        "; been implemented yet.  per-group is far from perfect but it does\n"
        "; seem to be a good enough approximation most of the time.\n"
        "gfx.rend.oit-mode per-group\n"
        "\n"
        "; set this to true to mute audio.  Set it to false to allow audio \n"
        "; to play\n"
        "audio.mute true\n"
        "\n"
        "; don't change this line.  It doesn't techincally do anything yet\n"
        "; but it will in future revisions of WashingtonDC.\n"
        "wash.dc.port.0.0 dreamcast_controller\n"
        "\n"
        /*
         * TODO: find a way to explain the naming convention for control
         * bindings to end-users
         */
        "wash.ctrl.toggle-overlay kbd.f2\n"
        "wash.ctrl.toggle-filter kbd.f3\n"
        "wash.ctrl.toggle-wireframe kbd.f4\n"

        "wash.ctrl.resume-execution kbd.f5\n"
        "wash.ctrl.run-one-frame kbd.f6\n"
        "wash.ctrl.pause-execution kbd.f7\n"

        "wash.ctrl.toggle-mute kbd.f8\n"
        "wash.ctrl.toggle-fullscreen kbd.f11\n"
        "wash.ctrl.screenshot kbd.f12\n"
        "\n"
        "; this bind can be used to immediately terminate WashingtonDC\n"
        "; it's disabled by default to prevent users from pressing it by\n"
        "; mistake, but if you feel confident in your ability to never hit\n"
        "; the wrong key then feel free to use this bind.\n"
        "; wash.ctrl.exit kbd.f10\n"
        "\n"
        "dc.ctrl.p1.dpad-up     js0.hat0.up\n"
        "dc.ctrl.p1.dpad-left   js0.hat0.left\n"
        "dc.ctrl.p1.dpad-down   js0.hat0.down\n"
        "dc.ctrl.p1.dpad-right  js0.hat0.right\n"
        "dc.ctrl.p1.stick-left  js0.axis0-\n"
        "dc.ctrl.p1.stick-right js0.axis0+\n"
        "dc.ctrl.p1.stick-up    js0.axis1-\n"
        "dc.ctrl.p1.stick-down  js0.axis1+\n"
        "dc.ctrl.p1.trig-l      js0.axis2\n"
        "dc.ctrl.p1.trig-r      js0.axis5\n"
        "dc.ctrl.p1.btn-a       js0.btn0\n"
        "dc.ctrl.p1.btn-b       js0.btn1\n"
        "dc.ctrl.p1.btn-x       js0.btn2\n"
        "dc.ctrl.p1.btn-y       js0.btn3\n"
        "dc.ctrl.p1.btn-start   js0.btn7\n"
        "\n"
        "; the (1) versions of the controls are duplicates so you can have\n"
        "; the same control bound to more than one key on the host machine.\n"
        "; There is no (2), (3), etc but there might be someday in the\n"
        "; future.\n"
        "dc.ctrl.p1.stick-up(1)    kbd.w\n"
        "dc.ctrl.p1.stick-left(1)  kbd.a\n"
        "dc.ctrl.p1.stick-down(1)  kbd.s\n"
        "dc.ctrl.p1.stick-right(1) kbd.d\n"
        "dc.ctrl.p1.trig-l(1)      kbd.q\n"
        "dc.ctrl.p1.trig-r(1)      kbd.e\n"
        "dc.ctrl.p1.dpad-up(1)     kbd.up\n"
        "dc.ctrl.p1.dpad-left(1)   kbd.left\n"
        "dc.ctrl.p1.dpad-down(1)   kbd.down\n"
        "dc.ctrl.p1.dpad-right(1)  kbd.right\n"
        "dc.ctrl.p1.btn-a(1)       kbd.keypad2\n"
        "dc.ctrl.p1.btn-b(1)       kbd.keypad6\n"
        "dc.ctrl.p1.btn-x(1)       kbd.keypad4\n"
        "dc.ctrl.p1.btn-y(1)       kbd.keypad8\n"
        "dc.ctrl.p1.btn-start(1)   kbd.space\n";

    LOG_INFO("Attempting to create configuration directory \"%s\"\n",
             cfg_file_dir);
    if (mkdir(cfg_file_dir, S_IRUSR | S_IWUSR | S_IXUSR) != 0) {
        if (errno == EEXIST) {
            LOG_INFO("The directory already exists, I'm going to assume that's "
                     "a good thing...\n");
        } else {
            LOG_ERROR("Unable to create %s: %s\n", cfg_file_dir, strerror(errno));
            return;
        }
    }

    FILE *cfg_file = fopen(cfg_file_path, "w");
    if (!cfg_file) {
        LOG_ERROR("unable to create %s: %s\n", cfg_file_path, strerror(errno));
        return;
    }

    if (fputs(cfg_default, cfg_file) == EOF) {
        LOG_ERROR("Unable to write default config to %s\n", cfg_file_path);
    }

    fclose(cfg_file);
}

void cfg_init(void) {
    memset(&cfg_state, 0, sizeof(cfg_state));
    cfg_state.state = CFG_PARSE_PRE_KEY;

    char const *cfg_file_path = hostfile_cfg_file();
    char const *cfg_file_dir = hostfile_cfg_dir();

    if (cfg_file_dir)
        LOG_INFO("cfg directory is \"%s\"\n", cfg_file_dir);
    else
        LOG_ERROR("unable to determine cfg directory\n");

    if (cfg_file_path)
        LOG_INFO("Using cfg file \"%s\"\n", cfg_file_path);
    else
        LOG_ERROR("Unable to determine location of cfg file\n");

    fifo_init(&cfg_state.cfg_nodes);

    FILE *cfg_file = fopen(cfg_file_path, "r");

    if (!cfg_file) {
        cfg_create_default_config();
        cfg_file = fopen(cfg_file_path, "r");
    }

    if (cfg_file) {
        LOG_INFO("Parsing wash.cfg\n");
        for (;;) {
            int ch = fgetc(cfg_file);
            if (ch == EOF)
                break;
            cfg_put_char(ch);
        }
        cfg_put_char('\n'); // in case the last line doesn't end with newline
        fclose(cfg_file);
    } else {
        LOG_INFO("Unable to open wash.cfg; does it even exist?\n");
    }
}

void cfg_cleanup(void) {
    struct fifo_node *curs;

    while ((curs = fifo_pop(&cfg_state.cfg_nodes)) != NULL) {
        struct cfg_node *node = &FIFO_DEREF(curs, struct cfg_node, list_node);
        free(node);
    }
}

void cfg_put_char(char ch) {
    /*
     * special case - a null terminator counts as a newline so that any data
     * which does not end in a newline can be flushed.
     */
    if (ch == '\0')
        ch = '\n';

    /*
     * Very simple preprocessor - replace comments with whitespace and
     * otherwise don't modify the parser state
     */
    if (ch == ';')
        cfg_state.in_comment = true;
    if (cfg_state.in_comment) {
        if (ch == '\n')
            cfg_state.in_comment = false;
        else
            ch = ' ';
    }

    switch (cfg_state.state) {
    case CFG_PARSE_PRE_KEY:
        if (ch == '\n') {
            cfg_handle_newline();
        } else if (!isspace(ch)) {
            cfg_state.state = CFG_PARSE_KEY;
            cfg_state.key_len = 1;
            cfg_state.key[0] = ch;
        }
        break;
    case CFG_PARSE_KEY:
        if (ch == '\n') {
            LOG_ERROR("*** CFG ERROR INCOMPLETE LINE %u ***\n", cfg_state.line_count);
            cfg_handle_newline();
        } else if (isspace(ch)) {
            cfg_state.state = CFG_PARSE_PRE_VAL;
            cfg_state.key[cfg_state.key_len] = '\0';
        } else if (cfg_state.key_len < CFG_NODE_KEY_LEN - 1) {
            cfg_state.key[cfg_state.key_len++] = ch;
        } else {
            LOG_WARN("CFG file dropped char from line %u; key length is "
                     "limited to %u characters\n",
                     cfg_state.line_count, CFG_NODE_KEY_LEN - 1);
        }
        break;
    case CFG_PARSE_PRE_VAL:
        if (ch == '\n') {
            LOG_ERROR("*** CFG ERROR INCOMPLETE LINE %u ***\n", cfg_state.line_count);
            cfg_handle_newline();
        } else if (!isspace(ch)) {
            cfg_state.state = CFG_PARSE_VAL;
            cfg_state.val_len = 1;
            cfg_state.val[0] = ch;
        }
        break;
    case CFG_PARSE_VAL:
        if (ch == '\n') {
            cfg_state.val[cfg_state.val_len] = '\0';
            cfg_add_entry();
            cfg_handle_newline();
        } else if (isspace(ch)) {
            cfg_state.state = CFG_PARSE_POST_VAL;
            cfg_state.val[cfg_state.val_len] = '\0';
        } else if (cfg_state.val_len < CFG_NODE_VAL_LEN - 1) {
            cfg_state.val[cfg_state.val_len++] = ch;
        } else {
            LOG_WARN("CFG file dropped char from line %u; value length is "
                     "limited to %u characters\n",
                     cfg_state.line_count, CFG_NODE_VAL_LEN - 1);
        }
        break;
    case CFG_PARSE_POST_VAL:
        if (ch == '\n') {
            cfg_add_entry();
            cfg_handle_newline();
        } else if (!isspace(ch)) {
            cfg_state.state = CFG_PARSE_ERROR;
            LOG_ERROR("*** CFG ERROR INVALID DATA LINE %u ***\n", cfg_state.line_count);
        }
        break;
    default:
    case CFG_PARSE_ERROR:
        if (ch == '\n')
            cfg_handle_newline();
        break;
    }
}

static void cfg_add_entry(void) {
    struct cfg_node *dst_node = NULL;
    struct fifo_node *curs;

    FIFO_FOREACH(cfg_state.cfg_nodes, curs) {
        struct cfg_node *node = &FIFO_DEREF(curs, struct cfg_node, list_node);
        if (strcmp(node->key, cfg_state.key) == 0) {
            dst_node = node;
            break;
        }
    }

    if (dst_node) {
        LOG_INFO("CFG overwriting existing config key \"%s\" at line %u\n",
                 cfg_state.key, cfg_state.line_count);
    } else {
        LOG_INFO("CFG allocating new config key \"%s\" at line %u\n",
                 cfg_state.key, cfg_state.line_count);
        dst_node = (struct cfg_node*)malloc(sizeof(struct cfg_node));
        memcpy(dst_node->key, cfg_state.key, sizeof(dst_node->key));
        fifo_push(&cfg_state.cfg_nodes, &dst_node->list_node);
    }

    if (dst_node)
        memcpy(dst_node->val, cfg_state.val, sizeof(dst_node->val));
    else
        LOG_ERROR("CFG file dropped line %u due to failed node allocation\n", cfg_state.line_count);
}

static void cfg_handle_newline(void) {
    cfg_state.state = CFG_PARSE_PRE_KEY;
    cfg_state.key_len = 0;
    cfg_state.val_len = 0;
    cfg_state.line_count++;
}

char const *cfg_get_node(char const *key) {
    struct fifo_node *curs;

    FIFO_FOREACH(cfg_state.cfg_nodes, curs) {
        struct cfg_node *node = &FIFO_DEREF(curs, struct cfg_node, list_node);
        if (strcmp(node->key, key) == 0)
            return node->val;
    }

    return NULL;
}

static int cfg_parse_bool(char const *valstr, bool *outp) {
    if (strcmp(valstr, "true") == 0 || strcmp(valstr, "1") == 0) {
        *outp = true;
        return 0;
    } else if (strcmp(valstr, "false") == 0 || strcmp(valstr, "0") == 0) {
        *outp = false;
        return 0;
    }
    return -1;
}

int cfg_get_bool(char const *key, bool *outp) {
    char const *nodestr = cfg_get_node(key);
    if (nodestr) {
        int success = cfg_parse_bool(nodestr, outp);
        if (success != 0)
            LOG_ERROR("error parsing config node \"%s\"\n", key);
        return success;
    }
    return -1;
}

static int cfg_parse_rgb(char const *valstr, int *red, int *green, int *blue) {
    if (strlen(valstr) != 7)
        return -1;

    if (valstr[0] != '#')
        return -1;

    int idx;
    unsigned digits[6];

    for (idx = 0; idx < 6; idx++) {
        char ch = valstr[idx + 1];
        if (ch >= '0' && ch <= '9') {
            digits[idx] = ch - '0';
        } else if (ch >= 'a' && ch <= 'f') {
            digits[idx] = ch - 'a' + 10;
        } else if (ch >= 'A' && ch <= 'F') {
            digits[idx] = ch - 'A' + 10;
        } else {
            LOG_ERROR("Bad color syntax \"%s\"\n", valstr);
            return -1;
        }
    }

    *red = digits[0] * 16 + digits[1];
    *green = digits[2] * 16 + digits[3];
    *blue = digits[4] * 16 + digits[5];

    return 0;
}

static int cfg_parse_decimal_int(char const *valstr, int *val_out) {
    if (!*valstr)
        return -1; //empty string

    int sign = 1;
    if (valstr[0] == '-') {
        valstr++;
        sign = -1;
    } else if (valstr[0] == '+') {
        // you can prepend positive values with + but it won't do anything.
        valstr++;
    }

    size_t len = strlen(valstr);
    if (len == 0)
        return -1;

    int scale = 1;
    int sum = 0;
    do {
        int digit = valstr[len - 1];
        if (!isdigit(digit))
            return -1;
        digit -= '0';
        sum += scale * digit;
        scale *= 10;
    } while (--len);

    *val_out = sign * sum;
    return 0;
}

int cfg_get_rgb(char const *key, int *red, int *green, int *blue) {
    char const *nodestr = cfg_get_node(key);
    if (nodestr) {
        int success = cfg_parse_rgb(nodestr, red, green, blue);
        if (success != 0)
            LOG_ERROR("error parsing config node \"%s\"\n", key);
        return success;
    }
    return -1;
}

int cfg_get_int(char const *key, int *val) {
    char const *nodestr = cfg_get_node(key);

    if (nodestr) {
        int success = cfg_parse_decimal_int(nodestr, val);
        if (success != 0)
            LOG_ERROR("error parsing config node \"%s\"\n", key);
        return success;
    }
    return -1;
}
