#include "wind.h"

int xerror(Display *display, XErrorEvent *e) {
    (void)display;
    if (e->error_code == BadAccess) wm_error = 1;
    /* Benign races: a window can disappear between events and operations. */
    else if (e->error_code != BadWindow && e->error_code != BadDrawable) {
        char msg[128]; XGetErrorText(display,e->error_code,msg,sizeof msg);
        fprintf(stderr,"wind: X error: %s (request=%d, resource=0x%lx)\n",msg,e->request_code,e->resourceid);
    }
    return 0;
}
void handle_event(XEvent *e) {
    switch (e->type) {
    case MapRequest: manage(e->xmaprequest.window); break;
    case ConfigureRequest: {
        XConfigureRequestEvent *q=&e->xconfigurerequest;
        Client *c=find_client(q->window);
        if (c && q->window==c->win) {
            int x=(q->value_mask&CWX)?q->x:c->x;
            int y=(q->value_mask&CWY)?q->y:c->y;
            int w=(q->value_mask&CWWidth)?q->width:c->w;
            int h=(q->value_mask&CWHeight)?q->height:c->h;
            place_client(c,x,y,w,h);
        } else {
            XWindowChanges wc = {0};
            wc.x=q->x; wc.y=q->y; wc.width=q->width; wc.height=q->height;
            wc.border_width=q->border_width; wc.sibling=q->above; wc.stack_mode=q->detail;
            XConfigureWindow(dpy,q->window,q->value_mask,&wc);
        }
    } break;
    case DestroyNotify: {
        Client *c=find_client(e->xdestroywindow.window);
        if (c && c->win==e->xdestroywindow.window) unmanage(c,true,false);
    } break;
    case UnmapNotify: {
        Client *c=find_client(e->xunmap.window);
        if (c && c->win == e->xunmap.window) {
            if (c->ignore_unmap > 0) c->ignore_unmap--;
            else unmanage(c,false,false);
        }
    } break;
    case PropertyNotify: {
        Client *c=find_client(e->xproperty.window);
        if (c && (e->xproperty.atom == XA_WM_NAME || e->xproperty.atom == a_name)) { read_title(c); draw_frame(c); }
    } break;
    case Expose:
        if (e->xexpose.count) break;
        if (e->xexpose.window==panel) draw_panel();
        else if(e->xexpose.window==menu) draw_menu();
        else {Client*c=find_client(e->xexpose.window);if(c && c->frame==e->xexpose.window)draw_frame(c);}
        break;
    case ButtonPress: {
        XButtonEvent *b=&e->xbutton;
        if (menu_visible) {
            if (b->button==Button4) {menu_offset=MAX(0,menu_offset-2);draw_menu();break;}
            if (b->button==Button5) {menu_offset=MIN(MAX(0,entry_count-visible_items()),menu_offset+2);draw_menu();break;}
            if (b->window==menu || (b->x_root>=menu_x && b->x_root<menu_x+MENU_W && b->y_root>=menu_y && b->y_root<menu_y+menu_h)) {
                int idx=(b->y_root-menu_y-42)/ITEM_H;
                if (b->y_root-menu_y>=42 && idx>=0 && idx<visible_items()) select_menu_item(menu_offset+idx);
                else hide_menu();
            } else hide_menu();
            break;
        }
        if (b->window==root || b->window==panel) {
            if (b->button==Button1 || b->button==Button3)
                show_menu(b->x_root,b->y_root);
            break;
        }
        Client *c=find_client(b->window);
        if (c) {
            focus_client(c);
            /* No unmodified titlebar drags and no Super mouse movement.
             * Alt+Button1 moves; Alt+Button3 resizes. */
            unsigned int mod = b->state & ~(LockMask | numlockmask);
            if (mod == Mod1Mask && b->button == Button1)
                start_drag(c, b, false);
            else if (mod == Mod1Mask && b->button == Button3)
                start_drag(c, b, true);
        }
    } break;
    case MotionNotify:
        if (menu_visible) {
            int v=(e->xmotion.y_root-menu_y-42)/ITEM_H;
            if(e->xmotion.y_root<menu_y+42 || v>=visible_items()) v=-1;
            if(v!=menu_hover){menu_hover=v;draw_menu();}
        } else if (drag_client && drag_mode!=DRAG_NONE) {
            int dx=e->xmotion.x_root-drag_px,dy=e->xmotion.y_root-drag_py;
            if(drag_mode==DRAG_MOVE) {
                place_client(drag_client,drag_x+dx,drag_y+dy,drag_client->w,drag_client->h);
            } else place_client(drag_client,drag_x,drag_y,drag_w+dx,drag_h+dy);
        }
        break;
    case ButtonRelease:
        if(drag_mode!=DRAG_NONE) {
            XUngrabPointer(dpy,CurrentTime); drag_mode=DRAG_NONE;drag_client=NULL;
        }
        break;
    case EnterNotify:
        /* Ignore synthetic crossings from grabs/reparenting; About is a
         * normal client and must not block focus on other clients. */
        if (config.focus_follows_mouse && !menu_visible && drag_mode == DRAG_NONE &&
            e->xcrossing.mode == NotifyNormal &&
            e->xcrossing.detail != NotifyInferior) {
            Client *c = find_client(e->xcrossing.window);
            if (c && c != focused) focus_client_hover(c);
        }
        break;
    case MappingNotify:
        XRefreshKeyboardMapping(&e->xmapping);
        update_numlockmask();
        grab_keys();
        for (Client *c = clients; c; c = c->next) grab_buttons(c);
        break;
    case KeyPress: key_event(&e->xkey); break;
    case ConfigureNotify:
        if (e->xconfigure.window==root) {
            int w=DisplayWidth(dpy,screen),h=DisplayHeight(dpy,screen);
            if(sw!=w || sh!=h) {
                sw=w;sh=h;
                XMoveResizeWindow(dpy,panel,0,0,(unsigned)sw,BAR_H);
                draw_panel();
                for (Client *c=clients; c; c=c->next)
                    place_client(c, c->x, c->y, c->w, c->h);
                set_wallpaper();
            }
        }
        break;
    }
}

