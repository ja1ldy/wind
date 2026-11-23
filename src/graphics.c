#include "wind.h"

void copystr(char *dst, size_t n, const char *src) {
    if (n == 0) return;
    size_t len = src ? strlen(src) : 0;
    if (len >= n) len = n - 1;
    if (len) memcpy(dst, src, len);
    dst[len] = 0;
}

unsigned long color(const char *name) {
    XColor exact, screen_color;
    if (XAllocNamedColor(dpy, DefaultColormap(dpy, screen), name, &screen_color, &exact))
        return screen_color.pixel;
    return BlackPixel(dpy, screen);
}

static void set_fg(unsigned long pixel) { XSetForeground(dpy, gc, pixel); }
void rect(Window w, int x, int y, unsigned width, unsigned height, unsigned long pixel) {
    set_fg(pixel); XFillRectangle(dpy, w, gc, x, y, width, height);
}
void line(Window w, int x1, int y1, int x2, int y2, unsigned long pixel) {
    set_fg(pixel); XDrawLine(dpy, w, gc, x1, y1, x2, y2);
}
/* Xft and Fontconfig handle TrueType/OpenType fonts and UTF-8.
 * A single XftDraw is reused for each drawable to avoid per-frame allocations. */
void text_at(Window w, int x, int y, const char *s, unsigned long pixel) {
    if (!font || !font_draw || !s) return;
    char lowered[1024];
    size_t n = strlen(s);
    if (n >= sizeof lowered) n = sizeof lowered - 1;
    for (size_t i = 0; i < n; i++) lowered[i] = (char)tolower((unsigned char)s[i]);
    lowered[n] = '\0';
    XColor c = {.pixel = pixel};
    XQueryColor(dpy, DefaultColormap(dpy, screen), &c);
    XftColor ink = {.pixel = pixel, .color = {
        .red = c.red, .green = c.green, .blue = c.blue, .alpha = 0xffff
    }};
    XftDrawChange(font_draw, w);
    XftDrawStringUtf8(font_draw, &ink, font, x, y, (const FcChar8 *)lowered, (int)n);
}
int text_width(const char *s) {
    if (!font) return (int)strlen(s) * 8;
    XGlyphInfo extents;
    XftTextExtentsUtf8(dpy, font, (const FcChar8 *)s, (int)strlen(s), &extents);
    return extents.xOff;
}

/* SVG is the actual logo asset. This deliberately supports just the <line>
 * and <circle> primitives used by assets/wind-logo.svg; no SVG library needed.
 * Keeping the vector source as the single source of truth avoids duplicate art. */
typedef struct { int x1, y1, x2, y2; } LogoLine;
static LogoLine logo_lines[12];
static int logo_line_count, logo_circle_x, logo_circle_y, logo_circle_r;
static bool logo_circle;
void load_logo(void) {
    char path[1024];
    snprintf(path, sizeof path, "%s/assets/wind-logo.svg", WIND_DATA_DIR);
    FILE *f = fopen(path, "r");
    if (!f) f = fopen("assets/wind-logo.svg", "r");
    if (!f) return;
    logo_line_count = 0;
    logo_circle = false;
    char linebuf[256];
    while (fgets(linebuf, sizeof linebuf, f)) {
        LogoLine a;
        int x, y, r;
        if (sscanf(linebuf, " <line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\"",
                   &a.x1, &a.y1, &a.x2, &a.y2) == 4) {
            if (logo_line_count < (int)(sizeof logo_lines / sizeof logo_lines[0]))
                logo_lines[logo_line_count++] = a;
        } else if (sscanf(linebuf, " <circle cx=\"%d\" cy=\"%d\" r=\"%d\"",
                          &x, &y, &r) == 3) {
            logo_circle_x=x; logo_circle_y=y; logo_circle_r=r; logo_circle=true;
        }
    }
    fclose(f);
}
void draw_logo(Window w, int x, int y, int size) {
    if (!logo_line_count && !logo_circle) return;
    XSetForeground(dpy, gc, theme.p_accent);
    XSetLineAttributes(dpy, gc, MAX(1, size * 3 / 64), LineSolid, CapRound, JoinRound);
    for (int i = 0; i < logo_line_count; ++i) {
        LogoLine *a = &logo_lines[i];
        XDrawLine(dpy, w, gc,
                  x + a->x1 * size / 64, y + a->y1 * size / 64,
                  x + a->x2 * size / 64, y + a->y2 * size / 64);
    }
    if (logo_circle) {
        int r = MAX(1, logo_circle_r * size / 64);
        XFillArc(dpy, w, gc, x + logo_circle_x*size/64-r,
                 y + logo_circle_y*size/64-r, (unsigned)(r*2), (unsigned)(r*2), 0, 360*64);
    }
    XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
}

void draw_panel(void) {
    if (!panel || !panel_visible) return;
    rect(panel, 0, 0, (unsigned)sw, BAR_H, theme.p_panel);
    draw_logo(panel, 7, 2, 19);
    text_at(panel, 30, 16, "wind", theme.p_panel_fg);
    char date[100]; time_t t = time(NULL); struct tm tm;
    localtime_r(&t, &tm);
    strftime(date, sizeof date, "%d %b %H:%M", &tm);
    text_at(panel, MAX(110, sw-text_width(date)-12), 16, date, theme.p_panel_fg);
}

