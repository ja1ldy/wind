#include "wind.h"

Client *find_client(Window w) {
    for (Client *c = clients; c; c = c->next)
        if (c->win == w || c->frame == w) return c;
    return NULL;
}
void read_title(Client *c) {
    Atom actual; int format; unsigned long count, leftover; unsigned char *data = NULL;
    if (XGetWindowProperty(dpy, c->win, a_name, 0, 127, False, a_utf8,
                           &actual, &format, &count, &leftover, &data) == Success &&
        data && format == 8 && count) {
        size_t n = MIN((int)count, (int)sizeof(c->title)-1);
        memcpy(c->title, data, n); c->title[n] = 0; XFree(data); return;
    }
    if (data) XFree(data);
    char *name = NULL;
    if (XFetchName(dpy, c->win, &name) && name) {
        copystr(c->title, sizeof c->title, name); XFree(name);
    } else copystr(c->title, sizeof c->title, "application");
}

void draw_frame(Client *c) {
    if (!c) return;
    unsigned long bg = (c == focused) ? theme.p_title : theme.p_menu;
    rect(c->frame, 0, 0, (unsigned)c->w, TITLE_H, bg);
    rect(c->frame, 0, TITLE_H-1, (unsigned)c->w, 1, theme.p_frame);
    char title[98]; copystr(title, sizeof title, c->title);
    if (strlen(title) > 58) { title[56] = '.'; title[57] = '.'; title[58] = '.'; title[59] = 0; }
    text_at(c->frame, 9, 15, title, theme.p_title_fg);
}

/* Hover sets focus without raising: raising on EnterNotify can make
 * overlapping clients steal focus back and forth. */
static void focus_impl(Client *c, bool raise) {
    if (!c) return;
    if (c == focused) {
        if (raise) XRaiseWindow(dpy, c->frame);
        return;
    }
    Client *old = focused;
    focused = c;
    if (raise) XRaiseWindow(dpy, c->frame);
    if (old) {
        XSetWindowBorder(dpy, old->frame, theme.p_frame);
        draw_frame(old);
    }
    XSetWindowBorder(dpy, c->frame, theme.p_focus_border);
    XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
    draw_frame(c);
    if (raise && panel && panel_visible) XRaiseWindow(dpy, panel);
    if (raise && menu_visible) XRaiseWindow(dpy, menu);
}
void focus_client(Client *c) { focus_impl(c, true); }
void focus_client_hover(Client *c) { focus_impl(c, false); }

void send_configure(Client *c) {
    XEvent ev = {0};
    ev.xconfigure.type = ConfigureNotify;
    ev.xconfigure.display = dpy;
    ev.xconfigure.event = c->win;
    ev.xconfigure.window = c->win;
    ev.xconfigure.x = c->x;
    ev.xconfigure.y = c->y + TITLE_H;
    ev.xconfigure.width = c->w;
    ev.xconfigure.height = c->h;
    ev.xconfigure.border_width = 0;
    ev.xconfigure.above = None;
    ev.xconfigure.override_redirect = False;
    XSendEvent(dpy, c->win, False, StructureNotifyMask, &ev);
}
void place_client(Client *c, int x, int y, int w, int h) {
    if (!c) return;
    /* Keep the entire decorated frame on screen, accounting for its border,
     * titlebar and optional panel. Clamp both moves and resize requests. */
    const int top = panel_visible ? BAR_H + FRAME_BORDER : FRAME_BORDER;
    const int max_w = MAX(1, sw - 2*FRAME_BORDER);
    const int max_h = MAX(1, sh - top - TITLE_H - FRAME_BORDER);
    c->w = MIN(MAX(MIN_W, w), max_w);
    c->h = MIN(MAX(MIN_H, h), max_h);
    c->x = MAX(FRAME_BORDER, MIN(x, sw - c->w - FRAME_BORDER));
    c->y = MAX(top, MIN(y, sh - c->h - TITLE_H - FRAME_BORDER));
    XMoveResizeWindow(dpy, c->frame, c->x, c->y,
                      (unsigned)c->w, (unsigned)(c->h + TITLE_H));
    XMoveResizeWindow(dpy, c->win, 0, TITLE_H, (unsigned)c->w, (unsigned)c->h);
    send_configure(c);
    draw_frame(c);
}
void close_client(Client *c) {
    if (!c) return;
    Atom *protocols = NULL; int n = 0; bool supported = false;
    if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
        for (int i = 0; i < n; i++) if (protocols[i] == a_delete) supported = true;
        if (protocols) XFree(protocols);
    }
    if (supported) {
        XEvent ev = {0};
        ev.xclient.type = ClientMessage;
        ev.xclient.window = c->win;
        ev.xclient.message_type = a_protocols;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = a_delete;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, c->win, False, NoEventMask, &ev);
    } else XKillClient(dpy, c->win);
}

/* dwm-style passive button grabs. Clients receive ordinary clicks; only
 * Alt + Button1/Button3 is intercepted over the window or its frame.
 * Account for CapsLock and the actual NumLock modifier (not always Mod2). */
void update_numlockmask(void) {
    numlockmask = 0;
    XModifierKeymap *map = XGetModifierMapping(dpy);
    if (!map) return;
    KeyCode code = XKeysymToKeycode(dpy, XK_Num_Lock);
    for (int mod = 0; mod < 8; mod++)
        for (int k = 0; k < map->max_keypermod; k++)
            if (map->modifiermap[mod * map->max_keypermod + k] == code && code)
                numlockmask = 1U << mod;
    XFreeModifiermap(map);
}
void grab_buttons(Client *c) {
    const unsigned int modifiers[] = {0, LockMask, numlockmask, LockMask | numlockmask};
    const unsigned int mouse_mods[] = {Mod1Mask}; /* alt only */
    const Window targets[] = {c->win, c->frame};
    for (unsigned t = 0; t < sizeof targets / sizeof targets[0]; t++) {
        XUngrabButton(dpy, AnyButton, AnyModifier, targets[t]);
        for (unsigned m = 0; m < sizeof mouse_mods / sizeof mouse_mods[0]; m++)
            for (unsigned i = 0; i < sizeof modifiers / sizeof modifiers[0]; i++) {
                XGrabButton(dpy, Button1, mouse_mods[m] | modifiers[i],
                            targets[t], False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                            GrabModeAsync, GrabModeAsync, None, None);
                XGrabButton(dpy, Button3, mouse_mods[m] | modifiers[i],
                            targets[t], False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                            GrabModeAsync, GrabModeAsync, None, None);
            }
    }
}
/* Like dwm's movemouse/resizemouse: grab on the root so motion and release
 * keep reaching WindWM even after the pointer leaves the original window. */
void start_drag(Client *c, XButtonEvent *b, bool resize) {
    Cursor drag_cursor = resize ? cursor_resize : cursor_move;
    if (XGrabPointer(dpy, root, False, PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, drag_cursor, CurrentTime) != GrabSuccess)
        return;
    drag_mode = resize ? DRAG_RESIZE : DRAG_MOVE;
    drag_client = c;
    drag_px = b->x_root; drag_py = b->y_root;
    drag_x = c->x; drag_y = c->y;
    drag_w = c->w; drag_h = c->h;
}

void manage(Window w) {
    if (find_client(w) || w == panel || w == menu) return;
    XWindowAttributes a;
    if (!XGetWindowAttributes(dpy, w, &a) || a.override_redirect || a.class == InputOnly) return;
    Client *c = calloc(1, sizeof *c);
    if (!c) return;
    c->win = w;
    const int top = panel_visible ? BAR_H+FRAME_BORDER : FRAME_BORDER;
    c->w = MIN(MAX(a.width, MIN_W), MAX(1, sw-2*FRAME_BORDER));
    c->h = MIN(MAX(a.height, MIN_H), MAX(1, sh-top-TITLE_H-FRAME_BORDER));
    c->x = MAX(FRAME_BORDER, MIN(a.x, sw-c->w-FRAME_BORDER));
    c->y = MAX(top, MIN(a.y, sh-c->h-TITLE_H-FRAME_BORDER));
    c->ignore_unmap = (a.map_state != IsUnmapped) ? 2 : 0;
    c->frame = XCreateSimpleWindow(dpy, root, c->x, c->y,
                                   (unsigned)c->w, (unsigned)(c->h + TITLE_H),
                                   FRAME_BORDER, theme.p_frame, theme.p_title);
    XDefineCursor(dpy, c->frame, cursor_normal);
    XSelectInput(dpy, c->frame, ExposureMask | ButtonPressMask |
                 EnterWindowMask | SubstructureRedirectMask | SubstructureNotifyMask);
    XSelectInput(dpy, c->win, PropertyChangeMask | StructureNotifyMask | EnterWindowMask);
    XAddToSaveSet(dpy, c->win);
    XSetWindowBorderWidth(dpy, c->win, 0);
    XReparentWindow(dpy, c->win, c->frame, 0, TITLE_H);
    XMapWindow(dpy, c->frame);
    XMapWindow(dpy, c->win);
    grab_buttons(c);
    read_title(c);
    c->next = clients;
    clients = c;
    focus_client(c);
    send_configure(c);
}

void unmanage(Client *c, bool destroyed, bool exiting) {
    if (!c) return;
    if (drag_client == c) { drag_client=NULL; drag_mode=DRAG_NONE; XUngrabPointer(dpy,CurrentTime); }
    if (focused == c) focused = NULL;
    Client **link = &clients;
    while (*link && *link != c) link = &(*link)->next;
    if (*link) *link = c->next;
    if (!destroyed) {
        XUnmapWindow(dpy, c->frame);
        XReparentWindow(dpy, c->win, root, c->x, c->y + TITLE_H);
        XRemoveFromSaveSet(dpy, c->win);
        XSetWindowBorderWidth(dpy, c->win, 0);
        if (exiting) XMapWindow(dpy, c->win);
    }
    if (font_draw) XftDrawChange(font_draw, root);
    XDestroyWindow(dpy, c->frame);
    free(c);
    if (!focused && clients) focus_client(clients);
}

