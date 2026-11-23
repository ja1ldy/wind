#include "wind.h"

void spawn(const char *cmd) {
    if (!cmd || !*cmd) return;
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        /* Keep DISPLAY from nested Xephyr; do not launch via sudo. */
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    } else if (pid < 0) perror("wind: fork");
}

void set_wallpaper(void) {
    if (!*config.wallpaper || !strcmp(config.wallpaper, "none")) return;
    char source[1024];
    const char *preset = NULL;
    if (!strcmp(config.wallpaper, "default") || !strcmp(config.wallpaper, "sunflowers"))
        preset = "sunflowers";
    else if (!strcmp(config.wallpaper, "ridge")) preset = "ridge";
    else if (!strcmp(config.wallpaper, "pine")) preset = "pine";
    else if (!strcmp(config.wallpaper, "dusk")) preset = "dusk";
    if (preset) {
        snprintf(source, sizeof source, "%s/assets/%s.jpg", WIND_DATA_DIR, preset);
        if (access(source, R_OK) != 0)
            snprintf(source, sizeof source, "assets/%s.jpg", preset);
    } else if (config.wallpaper[0] == '~' && config.wallpaper[1] == '/') {
        snprintf(source, sizeof source, "%s/%s", getenv("HOME") ? getenv("HOME") : "", config.wallpaper + 2);
    } else copystr(source, sizeof source, config.wallpaper);
    if (access(source, R_OK) != 0) return;
    /* User-configured wallpaper path is quoted safely for the shell. */
    char quoted[2300] = "'";
    for (const char *p = source; *p && strlen(quoted) < sizeof quoted - 7; ++p) {
        size_t n = strlen(quoted);
        if (*p == '\'') { memcpy(quoted + n, "'\\''", 4); quoted[n+4] = 0; }
        else { quoted[n] = *p; quoted[n+1] = 0; }
    }
    strcat(quoted, "'");
    char cmd[2400];
    snprintf(cmd, sizeof cmd, "command -v feh >/dev/null 2>&1 && feh --no-fehbg --bg-fill %s", quoted);
    spawn(cmd);
}

