#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
static Display *d;
static Window root;
static void delay(int ms){struct timespec t={ms/1000,(long)(ms%1000)*1000000L}; nanosleep(&t,NULL); XSync(d,False);}
static void point(int x,int y) {XTestFakeMotionEvent(d,-1,x,y,0); XFlush(d);delay(70);}
static void button(unsigned b,int down){XTestFakeButtonEvent(d,b,down,0);XFlush(d);delay(40);}
static void alt(int down){XTestFakeKeyEvent(d,XKeysymToKeycode(d,XK_Alt_L),down,0);XFlush(d);delay(40);}
static void super(int down){XTestFakeKeyEvent(d,XKeysymToKeycode(d,XK_Super_L),down,0);XFlush(d);delay(40);}
static void shift(int down){XTestFakeKeyEvent(d,XKeysymToKeycode(d,XK_Shift_L),down,0);XFlush(d);delay(40);}
static Window get_parent(Window w){Window r,p,*c=0;unsigned n=0;if(!XQueryTree(d,w,&r,&p,&c,&n))return 0;if(c)XFree(c);return p;}
static void geo(Window w,int *x,int *y,int *wth,int *h){XWindowAttributes a;Window child;XGetWindowAttributes(d,w,&a);XTranslateCoordinates(d,w,root,0,0,x,y,&child);*wth=a.width;*h=a.height;}
static void window_name(Window w,char *b,size_t n){char *name=0;if(XFetchName(d,w,&name)&&name){snprintf(b,n,"%s",name);XFree(name);} else b[0]=0;}
static Window find_about(void){Window r,p,*children=0;unsigned n=0; Window target=0;
if(!XQueryTree(d,root,&r,&p,&children,&n))return 0;
for(unsigned i=0;i<n;i++){Window f=children[i];Window r1,p1,*inner=0;unsigned n1=0;if(XQueryTree(d,f,&r1,&p1,&inner,&n1)){
for(unsigned j=0;j<n1;j++){char name[100];window_name(inner[j],name,sizeof name);if(!strcmp(name,"about wind")){target=inner[j];break;}}if(inner)XFree(inner);}if(target)break;
}if(children)XFree(children);return target;}
/* Check client contents, not just the background pixel. X11 discards
 * covered parts of windows unless their owner handles Expose correctly. */
static unsigned long long client_checksum(Window w) {
    XWindowAttributes a;
    if(!XGetWindowAttributes(d,w,&a) || a.width<50 || a.height<50) return 0;
    XImage *img=XGetImage(d,w,20,20,(unsigned)(a.width-40),
                         (unsigned)(a.height-40),AllPlanes,ZPixmap);
    if(!img) return 0;
    unsigned long long hash=1469598103934665603ULL;
    for(int y=0;y<img->height;y+=2) for(int x=0;x<img->width;x+=2) {
        hash^=XGetPixel(img,x,y);
        hash*=1099511628211ULL;
    }
    XDestroyImage(img);
    return hash;
}
static int fail;
static void expect(int okay,const char *test){printf("%s: %s\n",okay?"PASS":"FAIL",test);if(!okay)fail++;}
int main(void){d=XOpenDisplay(NULL);if(!d){perror("XOpenDisplay");return 3;}root=DefaultRootWindow(d);
Window a=XCreateSimpleWindow(d,root,110,110,350,220,0,0,0x335577);
Window b=XCreateSimpleWindow(d,root,560,130,260,195,0,0,0x445566);
XStoreName(d,a,"wind-test-one");XStoreName(d,b,"wind-test-two");
XMapWindow(d,a);XMapWindow(d,b);XFlush(d);delay(500);
Window fa=get_parent(a),fb=get_parent(b);int x,y,w,h;
expect(fa!=root && fb!=root,"two windows managed in frames");
point(150,190);Window focused;int revert;XGetInputFocus(d,&focused,&revert);expect(focused==a,"hover focuses window one");
point(615,225);XGetInputFocus(d,&focused,&revert);expect(focused==b,"hover focuses window two");
int bx,by,bw,bh;geo(fb,&bx,&by,&bw,&bh);XImage *pix=XGetImage(d,root,bx-1,by+15,1,1,AllPlanes,ZPixmap);XColor cc={.pixel=pix?XGetPixel(pix,0,0):0}; if(pix)XDestroyImage(pix);XQueryColor(d,DefaultColormap(d,DefaultScreen(d)),&cc);
expect(cc.red>0xd000 && cc.green>0x8000 && cc.blue<0x8000,"focused border is orange-yellow");
geo(fa,&x,&y,&w,&h);int ox=x,oy=y,ow=w,oh=h;
point(x+30,y+9);button(1,1);point(x+90,y+65);button(1,0);geo(fa,&x,&y,&w,&h);
expect(x==ox && y==oy,"plain titlebar drag does not move");
point(x+40,y+55);super(1);button(1,1);point(x+100,y+100);button(1,0);super(0);geo(fa,&x,&y,&w,&h);
expect(x==ox && y==oy,"super+left drag does not move");
point(x+50,y+70);alt(1);button(1,1);point(x+130,y+150);button(1,0);alt(0);geo(fa,&x,&y,&w,&h);
expect(x>ox+35 && y>oy+35,"alt+left moves");
point(x+80,y+70);alt(1);button(3,1);point(x+170,y+140);button(3,0);alt(0);geo(fa,&x,&y,&w,&h);
expect(w>ow+30 && h>oh+30,"alt+right resizes");
point(x+30,y+65);alt(1);button(1,1);point(1023,767);button(1,0);alt(0);geo(fa,&x,&y,&w,&h);
expect(x>=0 && y>=0 && x+w<=1024 && y+h<=768,"drag right/bottom clamps frame");
point(x+30,y+65);alt(1);button(1,1);point(0,0);button(1,0);alt(0);geo(fa,&x,&y,&w,&h);
expect(x>=0 && y>=23 && x+w<=1024 && y+h<=768,"drag left/top clamps frame above panel");
/* Show menu from desktop then choose about: fourth entry below 42px heading. */
point(950,90);button(1,1);button(1,0);delay(100);
point(765,90+42+3*29+12);button(1,1);button(1,0);delay(220);
Window ab=find_about();expect(ab!=0,"about window opens");
if(ab){Window af=get_parent(ab);int ax,ay,aw,ah;geo(af,&ax,&ay,&aw,&ah);
point(ax+45,ay+55);alt(1);button(3,1);point(ax+120,ay+110);button(3,0);alt(0);geo(af,&ax,&ay,&aw,&ah);
expect(aw>=500 && ah>=350,"about can resize via alt+right");
XWindowAttributes aboutattr;
XGetWindowAttributes(d, ab, &aboutattr);
XImage *aboutpix = XGetImage(d,ab,aboutattr.width-5,aboutattr.height-5,1,1,AllPlanes,ZPixmap);
unsigned long aboutpixel = aboutpix ? XGetPixel(aboutpix,0,0) : 0;
if (aboutpix) XDestroyImage(aboutpix);
expect(aboutpixel != 0, "about repaints the newly exposed area after resize");
XClassHint hint={0};
int class_ok = XGetClassHint(d,ab,&hint) && hint.res_class &&
               !strcmp(hint.res_class,"wind-about");
expect(class_ok,"about is a separate X11 application with its own WM_CLASS");
if(hint.res_class)XFree(hint.res_class);
if(hint.res_name)XFree(hint.res_name);
unsigned long long before=client_checksum(ab);
Window cover=XCreateSimpleWindow(d,root,ax+55,ay+50,300,200,0,0,0x557799);
XStoreName(d,cover,"wind-test-cover");
XMapWindow(d,cover);XFlush(d);delay(300);
XUnmapWindow(d,cover);XFlush(d);delay(400);
unsigned long long after=client_checksum(ab);
expect(before!=0 && before==after,"about text survives covering and uncovering");
XDestroyWindow(d,cover);delay(100);
Window focus2;XGetInputFocus(d,&focus2,&revert);expect(focus2==ab,"about can hold focus");
point(45,150);XGetInputFocus(d,&focus2,&revert);
expect(focus2==a,"hover focus continues while about window exists");
point(ax+155,ay+120);delay(100);
XGetInputFocus(d,&focus2,&revert);expect(focus2==ab,"about regains hover focus");
super(1);shift(1);
XTestFakeKeyEvent(d,XKeysymToKeycode(d,XK_c),1,0);
XTestFakeKeyEvent(d,XKeysymToKeycode(d,XK_c),0,0);
XFlush(d);delay(80);shift(0);super(0);delay(240);
expect(find_about()==0,"super+shift+c closes external about via WM_DELETE_WINDOW");
}
XDestroyWindow(d,a);XDestroyWindow(d,b);XFlush(d);XCloseDisplay(d);return fail?1:0;}
