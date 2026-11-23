#define _POSIX_C_SOURCE 200809L
/* wind-about: an independent, minimal X11 client for WindWM.
 * It handles Expose, ConfigureNotify and WM_DELETE_WINDOW itself.
 * Dependencies: Xlib, Xft. No toolkit, Lua or compositor.
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#ifndef WIND_DATA_DIR
#define WIND_DATA_DIR "/usr/local/share/windwm"
#endif
#define ABOUT_W 460
#define ABOUT_H 295
#define MAX(a,b) ((a)>(b)?(a):(b))

typedef struct { int x1, y1, x2, y2; } LogoLine;
static LogoLine logo_lines[12];
static int line_count, logo_x, logo_y, logo_radius, has_circle;

static Display *d;
static int screen;
static Window window;
static GC gc;
static Pixmap back;
static unsigned bw, bh;
static XftDraw *draw;
static XftFont *font;
static Atom wm_delete, wm_protocols;
static unsigned long bg, fg, accent, rule;
static XftColor ink_fg, ink_accent;
static const char *font_name = "GoMono Nerd Font Mono:size=11";
static const char *theme_name = "noir";
static const char *background_name = "#181d20";
static const char *foreground_name = "#f1f2ef";
static const char *accent_name = "#f1ad52";
static const char *rule_name = "#6f8b8b";

static unsigned long named_color(const char *name) {
    XColor parsed, exact;
    if (XAllocNamedColor(d, DefaultColormap(d,screen), name, &parsed, &exact))
        return parsed.pixel;
    return BlackPixel(d,screen);
}
static XftColor xft_color(unsigned long pixel) {
    XColor c = {.pixel = pixel};
    XQueryColor(d, DefaultColormap(d,screen), &c);
    XftColor x = {.pixel = pixel, .color = {
        .red=c.red, .green=c.green, .blue=c.blue, .alpha=0xffff
    }};
    return x;
}
static void rectangle(int x,int y,unsigned w,unsigned h,unsigned long pixel) {
    XSetForeground(d,gc,pixel);
    XFillRectangle(d,back,gc,x,y,w,h);
}
static void rule_line(int x1,int y1,int x2,int y2,unsigned long pixel) {
    XSetForeground(d,gc,pixel);
    XDrawLine(d,back,gc,x1,y1,x2,y2);
}
static void label(int x,int y,const char *s,const XftColor *ink) {
    if (!font || !draw || !s) return;
    /* All user-facing labels are lower-case, as in the WM. */
    char lower[256]; size_t n=strlen(s);
    if (n>=sizeof lower) n=sizeof lower-1;
    for(size_t i=0;i<n;i++) lower[i]=(char)tolower((unsigned char)s[i]);
    lower[n]=0;
    XftDrawStringUtf8(draw,ink,font,x,y,(const FcChar8 *)lower,(int)n);
}
static void load_logo(void) {
    char path[1024];
    snprintf(path,sizeof path,"%s/assets/wind-logo.svg",WIND_DATA_DIR);
    FILE *f=fopen(path,"r");
    if(!f) f=fopen("assets/wind-logo.svg","r");
    if(!f) return;
    char buf[256];
    while(fgets(buf,sizeof buf,f)) {
        LogoLine a; int x,y,r;
        if(sscanf(buf," <line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\"",
                  &a.x1,&a.y1,&a.x2,&a.y2)==4) {
            if(line_count<(int)(sizeof logo_lines/sizeof logo_lines[0])) logo_lines[line_count++]=a;
        } else if(sscanf(buf," <circle cx=\"%d\" cy=\"%d\" r=\"%d\"",&x,&y,&r)==3) {
            logo_x=x;logo_y=y;logo_radius=r;has_circle=1;
        }
    }
    fclose(f);
}
static void paint_logo(int x,int y,int size) {
    XSetForeground(d,gc,accent);
    XSetLineAttributes(d,gc,MAX(1,size*3/64),LineSolid,CapRound,JoinRound);
    for(int i=0;i<line_count;i++) {
        LogoLine *p=&logo_lines[i];
        XDrawLine(d,back,gc,x+p->x1*size/64,y+p->y1*size/64,
                              x+p->x2*size/64,y+p->y2*size/64);
    }
    if(has_circle) {
        int r=MAX(1,logo_radius*size/64);
        XFillArc(d,back,gc,x+logo_x*size/64-r,y+logo_y*size/64-r,
                 (unsigned)(2*r),(unsigned)(2*r),0,360*64);
    }
    XSetLineAttributes(d,gc,1,LineSolid,CapButt,JoinMiter);
}
static void read_os(char *out,size_t capacity) {
    snprintf(out,capacity,"linux");
    FILE *f=fopen("/etc/os-release","r");
    if(!f) return;
    char buf[256];
    while(fgets(buf,sizeof buf,f)) {
        if(strncmp(buf,"PRETTY_NAME=",12)) continue;
        char *s=buf+12;
        s[strcspn(s,"\r\n")]=0;
        if(*s=='\"') {s++; char *end=strrchr(s,'\"');if(end)*end=0;}
        size_t n=strlen(s);
        if(n>=capacity) n=capacity-1;
        memcpy(out,s,n); out[n]=0;
        break;
    }
    fclose(f);
}
static unsigned long mem_kib(void) {
    FILE *f=fopen("/proc/meminfo","r"); unsigned long kb=0;
    if(!f) return 0;
    if(fscanf(f,"MemTotal: %lu kB",&kb)!=1) kb=0;
    fclose(f);
    return kb;
}
static void ensure_buffer(unsigned w,unsigned h) {
    if(w==0 || h==0) return;
    if(back && bw==w && bh==h) return;
    if(draw) { XftDrawDestroy(draw); draw=NULL; }
    if(back) XFreePixmap(d,back);
    back=XCreatePixmap(d,window,w,h,(unsigned)DefaultDepth(d,screen));
    bw=w;bh=h;
    draw=XftDrawCreate(d,back,DefaultVisual(d,screen),DefaultColormap(d,screen));
}
static void paint(void) {
    XWindowAttributes attrs;
    if(!XGetWindowAttributes(d,window,&attrs) || attrs.width<=0 || attrs.height<=0) return;
    ensure_buffer((unsigned)attrs.width,(unsigned)attrs.height);
    if(!back) return;
    rectangle(0,0,bw,bh,bg);
    paint_logo(23,23,70);
    label(110,46,"windwm",&ink_accent);
    label(110,68,"x11 window manager  /  c + lua",&ink_fg);
    label(110,88,"version 0.3.2",&ink_fg);
    if(bw>40) rule_line(20,112,(int)bw-20,112,rule);
    struct utsname un={0};
    uname(&un);
    char os[128],linebuf[256];
    read_os(os,sizeof os);
    snprintf(linebuf,sizeof linebuf,"os        %.62s",os); label(25,139,linebuf,&ink_fg);
    snprintf(linebuf,sizeof linebuf,"kernel    %.58s",un.release); label(25,164,linebuf,&ink_fg);
    snprintf(linebuf,sizeof linebuf,"machine   %.58s",un.machine); label(25,189,linebuf,&ink_fg);
    snprintf(linebuf,sizeof linebuf,"display   %dx%d",DisplayWidth(d,screen),DisplayHeight(d,screen));
    label(25,214,linebuf,&ink_fg);
    snprintf(linebuf,sizeof linebuf,"memory    %.2f gib",mem_kib()/1048576.0);
    label(25,239,linebuf,&ink_fg);
    snprintf(linebuf,sizeof linebuf,"theme     %.28s",theme_name); label(25,264,linebuf,&ink_fg);
    XCopyArea(d,back,window,gc,0,0,bw,bh,0,0);
    XFlush(d);
}
static void usage(void) {
    puts("wind-about 0.3.2\nusage: wind-about [--font FONT] [--background COLOR]"
         " [--foreground COLOR] [--accent COLOR] [--rule COLOR] [--theme NAME]");
}
int main(int argc,char **argv) {
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--help")) {usage();return 0;}
        if(!strcmp(argv[i],"--version")){puts("wind-about 0.3.2");return 0;}
        if(i+1>=argc) {usage();return 2;}
        const char *val=argv[++i];
        if(!strcmp(argv[i-1],"--background")) background_name=val;
        else if(!strcmp(argv[i-1],"--foreground")) foreground_name=val;
        else if(!strcmp(argv[i-1],"--accent")) accent_name=val;
        else if(!strcmp(argv[i-1],"--rule")) rule_name=val;
        else if(!strcmp(argv[i-1],"--font")) font_name=val;
        else if(!strcmp(argv[i-1],"--theme")) theme_name=val;
        else {usage();return 2;}
    }
    d=XOpenDisplay(NULL);
    if(!d){fprintf(stderr,"wind-about: cannot open DISPLAY\n");return 1;}
    screen=DefaultScreen(d);
    Window root=RootWindow(d,screen);
    int x=MAX(0,(DisplayWidth(d,screen)-ABOUT_W)/2);
    int y=MAX(24,(DisplayHeight(d,screen)-ABOUT_H)/2);
    bg=named_color(background_name);fg=named_color(foreground_name);
    accent=named_color(accent_name);rule=named_color(rule_name);
    ink_fg=xft_color(fg);ink_accent=xft_color(accent);
    window=XCreateSimpleWindow(d,root,x,y,ABOUT_W,ABOUT_H,0,rule,bg);
    XStoreName(d,window,"about wind");
    XClassHint hint={.res_name="wind-about",.res_class="wind-about"};
    XSetClassHint(d,window,&hint);
    XSizeHints size={.flags=PMinSize,.min_width=ABOUT_W,.min_height=ABOUT_H};
    XSetWMNormalHints(d,window,&size);
    wm_delete=XInternAtom(d,"WM_DELETE_WINDOW",False);
    wm_protocols=XInternAtom(d,"WM_PROTOCOLS",False);
    XSetWMProtocols(d,window,&wm_delete,1);
    Atom net_pid=XInternAtom(d,"_NET_WM_PID",False);
    unsigned long pid=(unsigned long)getpid();
    XChangeProperty(d,window,net_pid,XA_CARDINAL,32,PropModeReplace,(unsigned char *)&pid,1);
    XSelectInput(d,window,ExposureMask|StructureNotifyMask);
    gc=XCreateGC(d,window,0,NULL);
    font=XftFontOpenName(d,screen,font_name);
    if(!font)font=XftFontOpenName(d,screen,"monospace:size=11");
    if(!font)fprintf(stderr,"wind-about: no usable Xft font\n");
    load_logo();
    XMapWindow(d,window);
    int running=1;
    while(running) {
        XEvent ev;
        XNextEvent(d,&ev);
        switch(ev.type) {
            case Expose: if(ev.xexpose.count==0) paint(); break;
            case ConfigureNotify:
                /* Redraw the *whole* client, including newly uncovered edges. */
                if(ev.xconfigure.window==window) paint();
                break;
            case ClientMessage:
                if(ev.xclient.message_type==wm_protocols &&
                   (Atom)ev.xclient.data.l[0]==wm_delete) running=0;
                break;
            case DestroyNotify: running=0;break;
        }
    }
    if(draw) XftDrawDestroy(draw);
    if(back) XFreePixmap(d,back);
    if(font) XftFontClose(d,font);
    XFreeGC(d,gc);
    XDestroyWindow(d,window);
    XCloseDisplay(d);
    return 0;
}
