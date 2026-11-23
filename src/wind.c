#include "wind.h"

Display *dpy;
Window root, panel, menu;
int screen, sw, sh, running = 1;
GC gc;
Cursor cursor_normal, cursor_move, cursor_resize;
unsigned int numlockmask;
XftFont *font;
XftDraw *font_draw;
Atom a_protocols, a_delete, a_name, a_utf8;
Client *clients, *focused;
Config config;
Theme theme;
MenuEntry entries[MAX_ENTRIES];
int entry_count, menu_offset, menu_visible, menu_hover = -1;
int menu_x, menu_y, menu_h;
bool panel_visible = true;
char config_path[768];
int wm_error;
DragMode drag_mode;
Client *drag_client;
int drag_px, drag_py, drag_x, drag_y, drag_w, drag_h;

static void manage_existing(void) {
    Window root_ret, parent, *children=NULL; unsigned n=0;
    if(!XQueryTree(dpy,root,&root_ret,&parent,&children,&n))return;
    for(unsigned i=0;i<n;i++) {
        XWindowAttributes a;
        if(XGetWindowAttributes(dpy,children[i],&a) && a.map_state!=IsUnmapped && !a.override_redirect)
            manage(children[i]);
    }
    if(children)XFree(children);
}
static void cleanup(void) {
    hide_menu();
    free_menu_buffer();
    while(clients)unmanage(clients,false,true);
    if(font_draw) { XftDrawDestroy(font_draw); font_draw=NULL; }
    if(panel)XDestroyWindow(dpy,panel);
    if(menu)XDestroyWindow(dpy,menu);
    if(font)XftFontClose(dpy,font);
    if(gc)XFreeGC(dpy,gc);
    if(cursor_normal) {
        XUndefineCursor(dpy, root);
        XFreeCursor(dpy, cursor_normal);
    }
    if(cursor_move) XFreeCursor(dpy, cursor_move);
    if(cursor_resize) XFreeCursor(dpy, cursor_resize);
    XCloseDisplay(dpy);
}
static void print_usage(const char *argv0) {
    printf("wind %s\nusage: %s [--config FILE] [--version] [--help]\n"
           "native x11 wm. start in xephyr or a dedicated xorg session.\n", VERSION,argv0);
}
int main(int argc,char **argv) {
    const char *home=getenv("HOME");
    snprintf(config_path,sizeof config_path,"%s/.config/wind/config.lua",home?home:".");
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--version")){puts("wind " VERSION);return 0;}
        if(!strcmp(argv[i],"--help")){print_usage(argv[0]);return 0;}
        if(!strcmp(argv[i],"--config") && i+1<argc) {copystr(config_path,sizeof config_path,argv[++i]);continue;}
        fprintf(stderr,"wind: unknown option: %s\n",argv[i]);return 2;
    }
    signal(SIGCHLD,SIG_IGN);
    dpy=XOpenDisplay(NULL);
    if(!dpy){fprintf(stderr,"wind: cannot open DISPLAY; use Xephyr or startx\n");return 1;}
    screen=DefaultScreen(dpy);root=RootWindow(dpy,screen);
    sw=DisplayWidth(dpy,screen);sh=DisplayHeight(dpy,screen);
    gc=XCreateGC(dpy,root,0,NULL);
    font_draw=XftDrawCreate(dpy,root,DefaultVisual(dpy,screen),DefaultColormap(dpy,screen));
    load_logo();
    load_config();
    XSetErrorHandler(xerror);
    XSelectInput(dpy,root,SubstructureRedirectMask|SubstructureNotifyMask|
                 ButtonPressMask|StructureNotifyMask|PropertyChangeMask);
    XSync(dpy,False);
    if(wm_error){fprintf(stderr,"wind: another X11 window manager is already running on %s\n",DisplayString(dpy));cleanup();return 1;}
    cursor_normal = XCreateFontCursor(dpy, XC_left_ptr);
    cursor_move = XCreateFontCursor(dpy, XC_fleur);
    cursor_resize = XCreateFontCursor(dpy, XC_bottom_right_corner);
    XDefineCursor(dpy, root, cursor_normal);
    update_numlockmask();
    a_protocols=XInternAtom(dpy,"WM_PROTOCOLS",False);
    a_delete=XInternAtom(dpy,"WM_DELETE_WINDOW",False);
    a_name=XInternAtom(dpy,"_NET_WM_NAME",False);
    a_utf8=XInternAtom(dpy,"UTF8_STRING",False);
    panel=XCreateSimpleWindow(dpy,root,0,0,(unsigned)sw,BAR_H,0,theme.p_panel,theme.p_panel);
    menu=XCreateSimpleWindow(dpy,root,0,BAR_H,MENU_W,200,1,theme.p_accent,theme.p_menu);
    XSetWindowAttributes wa={0}; wa.override_redirect=True;
    XChangeWindowAttributes(dpy,panel,CWOverrideRedirect,&wa);
    XChangeWindowAttributes(dpy,menu,CWOverrideRedirect,&wa);
    XDefineCursor(dpy, panel, cursor_normal);
    XDefineCursor(dpy, menu, cursor_normal);
    XSelectInput(dpy,panel,ExposureMask|ButtonPressMask);
    XSelectInput(dpy,menu,ExposureMask|ButtonPressMask|PointerMotionMask|KeyPressMask);
    XMapRaised(dpy,panel);
    grab_keys();
    manage_existing();
    set_wallpaper();
    draw_panel();
    fprintf(stderr,"wind %s on %s (%dx%d), theme=%s\n",VERSION,DisplayString(dpy),sw,sh,config.theme);
    while(running) {
        while(XPending(dpy)) {XEvent e;XNextEvent(dpy,&e);handle_event(&e);}
        fd_set fds;FD_ZERO(&fds);FD_SET(ConnectionNumber(dpy),&fds);
        struct timeval tv={.tv_sec=1,.tv_usec=0};
        int rc=select(ConnectionNumber(dpy)+1,&fds,NULL,NULL,&tv);
        if(rc==0)draw_panel();
        else if(rc<0 && errno!=EINTR){perror("wind: select");break;}
    }
    cleanup();return 0;
}
