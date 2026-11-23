#ifndef WIND_H
#define WIND_H
/*
 * WindWM (wind) - small reparenting X11 window manager.
 * Copyright (c) 2026 WindWM contributors. MIT license; see LICENSE.
 *
 * Deliberately uses plain Xlib and Lua. No compositor or toolkit.
 * A learning-sized, usable prototype rather than a complete EWMH WM.
 */
#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <ctype.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef WIND_DATA_DIR
#define WIND_DATA_DIR "/usr/local/share/windwm"
#endif
#define VERSION "0.3.2"
#define BAR_H 23
#define TITLE_H 22
#define FRAME_BORDER 1
#define MENU_W 274
#define ITEM_H 29
#define MAX_ENTRIES 256
#define MIN_W 100
#define MIN_H 48
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct Client Client;
struct Client {
    Window win, frame;
    int x, y, w, h;
    int ignore_unmap;
    char title[192];
    Client *next;
};

typedef enum { ACT_RUN, ACT_ABOUT, ACT_RELOAD, ACT_THEME, ACT_WALLPAPER, ACT_EXIT } Action;
typedef struct { char label[100], cmd[512]; Action action; } MenuEntry;
typedef struct {
    char theme[32], terminal[160], browser[160], file_manager[160];
    char wallpaper[768], font[128];
    int menu_items;
    bool focus_follows_mouse;
} Config;
typedef struct {
    char panel[24], panel_fg[24], background[24], accent[24];
    char focus_border[24], frame[24], title[24], title_fg[24], menu[24], menu_fg[24], highlight[24];
    unsigned long p_panel, p_panel_fg, p_bg, p_accent, p_frame, p_focus_border;
    unsigned long p_title, p_title_fg, p_menu, p_menu_fg, p_highlight;
} Theme;


extern Display *dpy;
extern Window root, panel, menu;
extern int screen, sw, sh, running;
extern GC gc;
extern Cursor cursor_normal, cursor_move, cursor_resize;
extern unsigned int numlockmask;
extern XftFont *font;
extern XftDraw *font_draw;
extern Atom a_protocols, a_delete, a_name, a_utf8;
extern Client *clients, *focused;
extern Config config;
extern Theme theme;
extern MenuEntry entries[MAX_ENTRIES];
extern int entry_count, menu_offset, menu_visible, menu_hover;
extern int menu_x, menu_y, menu_h;
extern bool panel_visible;
extern char config_path[768];
extern int wm_error;
typedef enum { DRAG_NONE, DRAG_MOVE, DRAG_RESIZE } DragMode;
extern DragMode drag_mode;
extern Client *drag_client;
extern int drag_px, drag_py, drag_x, drag_y, drag_w, drag_h;
void copystr(char *, size_t, const char *);
unsigned long color(const char *);
void rect(Window, int, int, unsigned, unsigned, unsigned long);
void line(Window, int, int, int, int, unsigned long);
void text_at(Window, int, int, const char *, unsigned long);
int text_width(const char *);
void load_logo(void);
void draw_logo(Window, int, int, int);
void draw_panel(void);
void load_config(void);
void reload_config(void);
void spawn(const char *);
void set_wallpaper(void);
Client *find_client(Window);
void read_title(Client *);
void draw_frame(Client *);
void focus_client(Client *);
void focus_client_hover(Client *);
void send_configure(Client *);
void place_client(Client *, int, int, int, int);
void close_client(Client *);
void update_numlockmask(void);
void grab_buttons(Client *);
void start_drag(Client *, XButtonEvent *, bool);
void manage(Window);
void unmanage(Client *, bool, bool);
int visible_items(void);
void hide_menu(void);
void show_menu(int, int);
void draw_menu(void);
void free_menu_buffer(void);
void show_about(void);
void select_menu_item(int);
void grab_keys(void);
void key_event(XKeyEvent *);
int xerror(Display *, XErrorEvent *);
void handle_event(XEvent *);

#endif
