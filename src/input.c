#include "wind.h"

/* Uses dwm defaults with Super as MODKEY instead of Alt.
 * WindWM is a floating WM, so tiling-specific dwm actions aren't bound. */
void grab_keys(void) {
    struct { KeySym sym; unsigned mod; } keys[] = {
        {XK_Return, Mod4Mask|ShiftMask}, /* terminal */
        {XK_p, Mod4Mask},                /* applications menu */
        {XK_j, Mod4Mask}, {XK_k, Mod4Mask}, /* focus stack */
        {XK_c, Mod4Mask|ShiftMask},      /* kill client */
        {XK_q, Mod4Mask|ShiftMask},      /* quit */
        {XK_b, Mod4Mask},                /* toggle bar */
        {XK_r, Mod4Mask|ShiftMask}       /* WindWM extension: reload Lua */
    };
    unsigned ignored[] = {0, LockMask, numlockmask, LockMask|numlockmask};
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    for (unsigned i=0; i<sizeof keys/sizeof keys[0]; ++i) {
        KeyCode k=XKeysymToKeycode(dpy,keys[i].sym);
        if (!k) continue;
        for(unsigned j=0;j<sizeof ignored/sizeof ignored[0];j++)
            XGrabKey(dpy,k,keys[i].mod|ignored[j],root,False,GrabModeAsync,GrabModeAsync);
    }
}
static void focus_stack(int direction) {
    if (!clients) return;
    Client *next = NULL;
    if (direction > 0) next = focused && focused->next ? focused->next : clients;
    else {
        for (Client *c=clients; c; c=c->next)
            if (c->next == focused) {next=c;break;}
        if (!next) {for (Client *c=clients; c; c=c->next) next=c;}
    }
    if (next) focus_client(next);
}
void key_event(XKeyEvent *e) {
    KeySym sym=XLookupKeysym(e,0);
    unsigned m=e->state & ~(LockMask|Mod2Mask);
    if (menu_visible) {
        if (sym==XK_Escape) { hide_menu(); return; }
        if (sym==XK_Down || sym==XK_Up) {
            int delta=(sym==XK_Down)?1:-1;
            menu_hover=MAX(0,MIN(visible_items()-1,menu_hover+delta));
            draw_menu(); return;
        }
        if (sym==XK_Return && menu_hover>=0) {
            select_menu_item(menu_offset+menu_hover); return;
        }
    }
    if (m==(Mod4Mask|ShiftMask) && sym==XK_q) running=0;
    else if (m==(Mod4Mask|ShiftMask) && sym==XK_r) reload_config();
    else if (m==(Mod4Mask|ShiftMask) && sym==XK_Return) spawn(config.terminal);
    else if (m==Mod4Mask && sym==XK_p) show_menu(8,panel_visible ? BAR_H+4 : 8);
    else if (m==(Mod4Mask|ShiftMask) && sym==XK_c) close_client(focused);
    else if (m==Mod4Mask && sym==XK_j) focus_stack(1);
    else if (m==Mod4Mask && sym==XK_k) focus_stack(-1);
    else if (m==Mod4Mask && sym==XK_b) {
        panel_visible=!panel_visible;
        if (panel_visible) { XMapRaised(dpy,panel); draw_panel(); }
        else XUnmapWindow(dpy,panel);
    }
}

