#include "wind.h"

/* The menu is fully drawn off-screen and presented in one XCopyArea.
 * A cached pixmap prevents repeated allocation on each pointer motion. */
static Pixmap menu_buffer;
static unsigned int buffer_height;

void free_menu_buffer(void) {
    if (!menu_buffer) return;
    if (font_draw) XftDrawChange(font_draw, root);
    XFreePixmap(dpy, menu_buffer);
    menu_buffer = None;
    buffer_height = 0;
}

int visible_items(void) {
    int max_on_screen = MAX(1, (sh - BAR_H - 42 - 30 - 8) / ITEM_H);
    return MIN(MIN(config.menu_items, max_on_screen), entry_count);
}
void hide_menu(void) {
    if (!menu_visible) return;
    XUngrabPointer(dpy, CurrentTime);
    XUngrabKeyboard(dpy, CurrentTime);
    XUnmapWindow(dpy, menu);
    menu_visible = 0; menu_hover = -1;
}
void show_menu(int x, int y) {
    menu_h = 42 + visible_items()*ITEM_H + 30;
    x = MAX(1, MIN(x, sw-MENU_W-3));
    y = MAX(panel_visible ? BAR_H+1 : 1, MIN(y, sh-menu_h-3));
    menu_x=x; menu_y=y;
    XMoveResizeWindow(dpy, menu, x, y, MENU_W, (unsigned)menu_h);
    XMapRaised(dpy, menu);
    menu_visible=1; menu_hover=-1;
    draw_menu();
    XGrabKeyboard(dpy, menu, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(dpy, menu, False,
                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
}
void draw_menu(void) {
    if (!menu_visible) return;
    if (!menu_buffer || buffer_height != (unsigned)menu_h) {
        free_menu_buffer();
        menu_buffer = XCreatePixmap(dpy, menu, MENU_W, (unsigned)menu_h,
                                    (unsigned)DefaultDepth(dpy, screen));
        buffer_height = (unsigned)menu_h;
    }
    Drawable target = menu_buffer;
    rect(target, 0, 0, MENU_W, (unsigned)menu_h, theme.p_menu);
    rect(target, 0, 0, MENU_W, 38, theme.p_panel);
    draw_logo(target, 8, 3, 33);
    text_at(target, 45, 24, "applications", theme.p_panel_fg);
    for (int i = 0; i < visible_items(); i++) {
        int idx = i+menu_offset, y = 42 + i*ITEM_H;
        if (idx >= entry_count) break;
        if (i == menu_hover) rect(target, 4, y, MENU_W-8, ITEM_H-1, theme.p_highlight);
        char label[48];
        copystr(label, sizeof label, entries[idx].label);
        if (strlen(label) > 29) { label[27]='.'; label[28]='.'; label[29]='.'; label[30]=0; }
        text_at(target, 17, y+19, label, theme.p_menu_fg);
    }
    line(target, 8, menu_h-30, MENU_W-8, menu_h-30, theme.p_frame);
    text_at(target, 13, menu_h-12,
            entry_count > visible_items() ? "wheel: scroll   esc: close" : "esc: close", theme.p_accent);
    XCopyArea(dpy, menu_buffer, menu, gc, 0, 0, MENU_W, (unsigned)menu_h, 0, 0);
    /* XftDraw must never still target a pixmap after we free it later. */
    if (font_draw) XftDrawChange(font_draw, root);
}

