#include "wind.h"
#include <X11/Xutil.h>

/* About is an ordinary X11 application, not a window rendered by the WM.
 * The application owns its event loop and repaints after exposure or resize. */
static bool is_about(const Client *c) {
    XClassHint hint = {0};
    if (!XGetClassHint(dpy, c->win, &hint)) return false;
    bool yes = (hint.res_class && !strcmp(hint.res_class, "wind-about"));
    if (hint.res_name) XFree(hint.res_name);
    if (hint.res_class) XFree(hint.res_class);
    return yes;
}

void show_about(void) {
    hide_menu();
    for (Client *c = clients; c; c = c->next) {
        if (is_about(c)) { focus_client(c); return; }
    }
    pid_t pid = fork();
    if (pid < 0) { perror("wind: launching wind-about"); return; }
    if (pid != 0) return;

    /* Keep the same DISPLAY, but don't reuse the parent's X connection. */
    setsid();
    close(ConnectionNumber(dpy));
    char *args[] = {
        "wind-about", "--background", theme.menu,
        "--foreground", theme.menu_fg,
        "--accent", theme.accent,
        "--rule", theme.frame,
        "--font", config.font,
        "--theme", config.theme,
        NULL
    };
    execvp(args[0], args);
    args[0] = "./wind-about";
    execv(args[0], args);
    perror("wind: cannot launch wind-about (run make and sudo make install)");
    _exit(127);
}
