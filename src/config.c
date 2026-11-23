#include "wind.h"

static const char *theme_names[] = {"sunset", "noir", "nord", "forest", "amber"};
static const int theme_count = 5;
static const char *wallpaper_names[] = {"sunflowers", "ridge", "pine", "dusk"};


static void load_defaults(void) {
    memset(&config, 0, sizeof(config));
    copystr(config.theme, sizeof config.theme, "noir");
    copystr(config.terminal, sizeof config.terminal, "x-terminal-emulator");
    copystr(config.browser, sizeof config.browser, "x-www-browser");
    copystr(config.file_manager, sizeof config.file_manager, "xdg-open .");
    copystr(config.wallpaper, sizeof config.wallpaper, "default");
    copystr(config.font, sizeof config.font, "GoMono Nerd Font Mono:size=11");
    config.menu_items = 15;
    config.focus_follows_mouse = true;
    memset(&theme, 0, sizeof(theme));
    copystr(theme.panel, sizeof theme.panel, "#10191c");
    copystr(theme.panel_fg, sizeof theme.panel_fg, "#e4eceb");
    copystr(theme.background, sizeof theme.background, "#183a40");
    copystr(theme.accent, sizeof theme.accent, "#f1ad52");
    copystr(theme.frame, sizeof theme.frame, "#6f8b8b");
    copystr(theme.focus_border, sizeof theme.focus_border, "#f0b24f");
    copystr(theme.title, sizeof theme.title, "#202628");
    copystr(theme.title_fg, sizeof theme.title_fg, "#f0f0eb");
    copystr(theme.menu, sizeof theme.menu, "#181d20");
    copystr(theme.menu_fg, sizeof theme.menu_fg, "#f1f2ef");
    copystr(theme.highlight, sizeof theme.highlight, "#36575e");
}

static void table_string(lua_State *L, const char *key, char *dest, size_t n) {
    lua_getfield(L, -1, key);
    if (lua_isstring(L, -1)) copystr(dest, n, lua_tostring(L, -1));
    lua_pop(L, 1);
}
static void table_boolean(lua_State *L, const char *key, bool *value) {
    lua_getfield(L, -1, key);
    if (lua_isboolean(L, -1)) *value = lua_toboolean(L, -1);
    lua_pop(L, 1);
}
static void table_int(lua_State *L, const char *key, int *value) {
    lua_getfield(L, -1, key);
    if (lua_isinteger(L, -1)) *value = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
}

static int load_lua_file(lua_State *L, const char *path) {
    if (luaL_dofile(L, path) == LUA_OK) return 1;
    fprintf(stderr, "wind: %s: %s\n", path, lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
}

static void add_menu(const char *label, const char *cmd, Action action) {
    if (entry_count >= MAX_ENTRIES || !label || !*label) return;
    for (int i = 0; i < entry_count; i++)
        if (!strcmp(entries[i].label, label)) return;
    MenuEntry *e = &entries[entry_count++];
    copystr(e->label, sizeof e->label, label);
    copystr(e->cmd, sizeof e->cmd, cmd);
    e->action = action;
}

static bool valid_theme_name(const char *name) {
    if (!*name || strlen(name) >= 32) return false;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p)
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || *p == '_' || *p == '-')) return false;
    return true;
}

/* Modest .desktop support: Name, Exec, Type, Hidden, NoDisplay, Terminal.
 * It deliberately does not claim full freedesktop Desktop Entry compliance. */
static void scan_desktop_dir(const char *dir) {
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d))) {
        size_t elen = strlen(ent->d_name);
        if (elen < 9 || strcmp(ent->d_name + elen - 8, ".desktop")) continue;
        char path[1024], name[100] = "", command[512] = "";
        char type[48] = "Application";
        bool hidden = false, in_entry = false, terminal = false;
        snprintf(path, sizeof path, "%s/%s", dir, ent->d_name);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        char linebuf[1024];
        while (fgets(linebuf, sizeof linebuf, f)) {
            char *line = linebuf;
            line[strcspn(line, "\r\n")] = 0;
            if (*line == '[') { in_entry = !strcmp(line, "[Desktop Entry]"); continue; }
            if (!in_entry || *line == '#' || !strchr(line, '=')) continue;
            char *eq = strchr(line, '='); *eq++ = 0;
            if (!strcmp(line, "Name")) copystr(name, sizeof name, eq);
            else if (!strcmp(line, "Exec")) copystr(command, sizeof command, eq);
            else if (!strcmp(line, "Type")) copystr(type, sizeof type, eq);
            else if (!strcmp(line, "Hidden") || !strcmp(line, "NoDisplay"))
                hidden |= !strcmp(eq, "true");
            else if (!strcmp(line, "Terminal")) terminal = !strcmp(eq, "true");
        }
        fclose(f);
        if (hidden || strcmp(type, "Application") || !*name || !*command) continue;
        /* Drop desktop Exec field codes (e.g. %F, %u, %%); handle common apps. */
        char clean[512]; size_t j = 0;
        for (size_t i = 0; command[i] && j < sizeof clean - 1; i++) {
            if (command[i] == '%' && command[i + 1]) {
                if (command[i + 1] == '%') clean[j++] = '%';
                i++; continue;
            }
            clean[j++] = command[i];
        }
        clean[j] = '\0';
        if (terminal) {
            char wrapped[512];
            snprintf(wrapped, sizeof wrapped, "%.140s -e %.340s", config.terminal, clean);
            add_menu(name, wrapped, ACT_RUN);
        } else add_menu(name, clean, ACT_RUN);
    }
    closedir(d);
}

void load_config(void) {
    load_defaults();
    entry_count = 0; menu_offset = 0;
    lua_State *L = luaL_newstate();
    if (!L) return;
    luaL_openlibs(L);
    if (access(config_path, R_OK) == 0 && load_lua_file(L, config_path)) {
        lua_getglobal(L, "wind");
        if (lua_istable(L, -1)) {
            table_string(L, "theme", config.theme, sizeof config.theme);
            table_string(L, "terminal", config.terminal, sizeof config.terminal);
            table_string(L, "browser", config.browser, sizeof config.browser);
            table_string(L, "file_manager", config.file_manager, sizeof config.file_manager);
            table_string(L, "wallpaper", config.wallpaper, sizeof config.wallpaper);
            table_string(L, "font", config.font, sizeof config.font);
            table_int(L, "menu_items", &config.menu_items);
            table_boolean(L, "focus_follows_mouse", &config.focus_follows_mouse);
        }
        lua_pop(L, 1);
    }
    config.menu_items = MAX(5, MIN(22, config.menu_items));
    if (!valid_theme_name(config.theme)) copystr(config.theme, sizeof config.theme, "noir");
    char path[1024];
    snprintf(path, sizeof path, "%s/themes/%s.lua", WIND_DATA_DIR, config.theme);
    if (access(path, R_OK) != 0)
        snprintf(path, sizeof path, "themes/%s.lua", config.theme);
    if (access(path, R_OK) == 0 && load_lua_file(L, path)) {
        lua_getglobal(L, "theme");
        if (lua_istable(L, -1)) {
            table_string(L, "panel", theme.panel, sizeof theme.panel);
            table_string(L, "panel_fg", theme.panel_fg, sizeof theme.panel_fg);
            table_string(L, "background", theme.background, sizeof theme.background);
            table_string(L, "accent", theme.accent, sizeof theme.accent);
            table_string(L, "frame", theme.frame, sizeof theme.frame);
            table_string(L, "focus_border", theme.focus_border, sizeof theme.focus_border);
            table_string(L, "title", theme.title, sizeof theme.title);
            table_string(L, "title_fg", theme.title_fg, sizeof theme.title_fg);
            table_string(L, "menu", theme.menu, sizeof theme.menu);
            table_string(L, "menu_fg", theme.menu_fg, sizeof theme.menu_fg);
            table_string(L, "highlight", theme.highlight, sizeof theme.highlight);
        }
        lua_pop(L, 1);
    }
    add_menu("terminal", config.terminal, ACT_RUN);
    add_menu("files", config.file_manager, ACT_RUN);
    add_menu("browser", config.browser, ACT_RUN);
    add_menu("about wind", "", ACT_ABOUT);
    add_menu("next theme", "", ACT_THEME);
    add_menu("next wallpaper", "", ACT_WALLPAPER);
    add_menu("reload", "", ACT_RELOAD);
    add_menu("quit wind", "", ACT_EXIT);
    /* A user's entries go before the auto-discovered system applications. */
    if (access(config_path, R_OK) == 0) {
        lua_getglobal(L, "wind");
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "apps");
            if (lua_istable(L, -1)) {
                lua_Integer n = luaL_len(L, -1);
                for (lua_Integer i = 1; i <= n && entry_count < MAX_ENTRIES; i++) {
                    lua_rawgeti(L, -1, i);
                    if (lua_istable(L, -1)) {
                        lua_rawgeti(L, -1, 1);
                        lua_rawgeti(L, -2, 2);
                        if (lua_isstring(L, -2) && lua_isstring(L, -1))
                            add_menu(lua_tostring(L, -2), lua_tostring(L, -1), ACT_RUN);
                        lua_pop(L, 2);
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    lua_close(L);
    scan_desktop_dir("/usr/share/applications");
    scan_desktop_dir("/usr/local/share/applications");
    const char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof path, "%s/.local/share/applications", home);
        scan_desktop_dir(path);
    }
    if (!dpy) return;
    theme.p_panel = color(theme.panel);
    theme.p_panel_fg = color(theme.panel_fg);
    theme.p_bg = color(theme.background);
    theme.p_accent = color(theme.accent);
    theme.p_frame = color(theme.frame);
    theme.p_focus_border = color(theme.focus_border);
    theme.p_title = color(theme.title);
    theme.p_title_fg = color(theme.title_fg);
    theme.p_menu = color(theme.menu);
    theme.p_menu_fg = color(theme.menu_fg);
    theme.p_highlight = color(theme.highlight);
    if (font) XftFontClose(dpy, font);
    font = XftFontOpenName(dpy, screen, config.font);
    if (!font) font = XftFontOpenName(dpy, screen, "monospace:size=11");
    XSetWindowBackground(dpy, root, theme.p_bg);
    XClearWindow(dpy, root);
}

void select_menu_item(int idx) {
    if (idx < 0 || idx >= entry_count) { hide_menu(); return; }
    MenuEntry selected = entries[idx];
    hide_menu();
    switch (selected.action) {
        case ACT_RUN: spawn(selected.cmd); break;
        case ACT_ABOUT: show_about(); break;
        case ACT_RELOAD: reload_config(); break;
        case ACT_THEME: {
            int idx2 = 0;
            for (int i=0; i<theme_count; i++)
                if (!strcmp(config.theme, theme_names[i])) { idx2=(i+1)%theme_count; break; }
            /* Preview the next preset; persists only if selected in config.lua. */
            char path[1024];
            snprintf(path, sizeof path, "%s/themes/%s.lua", WIND_DATA_DIR, theme_names[idx2]);
            if (access(path, R_OK) != 0) snprintf(path, sizeof path, "themes/%s.lua", theme_names[idx2]);
            lua_State *L=luaL_newstate();
            if (L) {
                luaL_openlibs(L);
                if (load_lua_file(L,path)) {
                    lua_getglobal(L,"theme");
                    if (lua_istable(L,-1)) {
                        table_string(L,"panel",theme.panel,sizeof theme.panel);
                        table_string(L,"panel_fg",theme.panel_fg,sizeof theme.panel_fg);
                        table_string(L,"background",theme.background,sizeof theme.background);
                        table_string(L,"accent",theme.accent,sizeof theme.accent);
                        table_string(L,"frame",theme.frame,sizeof theme.frame);
                        table_string(L,"focus_border",theme.focus_border,sizeof theme.focus_border);
                        table_string(L,"title",theme.title,sizeof theme.title);
                        table_string(L,"title_fg",theme.title_fg,sizeof theme.title_fg);
                        table_string(L,"menu",theme.menu,sizeof theme.menu);
                        table_string(L,"menu_fg",theme.menu_fg,sizeof theme.menu_fg);
                        table_string(L,"highlight",theme.highlight,sizeof theme.highlight);
                    }
                    lua_pop(L,1);
                    copystr(config.theme,sizeof config.theme,theme_names[idx2]);
                    theme.p_panel=color(theme.panel); theme.p_panel_fg=color(theme.panel_fg);
                    theme.p_bg=color(theme.background); theme.p_accent=color(theme.accent);
                    theme.p_frame=color(theme.frame); theme.p_focus_border=color(theme.focus_border); theme.p_title=color(theme.title);
                    theme.p_title_fg=color(theme.title_fg); theme.p_menu=color(theme.menu);
                    theme.p_menu_fg=color(theme.menu_fg); theme.p_highlight=color(theme.highlight);
                    XSetWindowBackground(dpy, root, theme.p_bg); XClearWindow(dpy,root);
                    XSetWindowBorder(dpy, menu, theme.p_accent);

                    draw_panel(); for(Client *c=clients;c;c=c->next) {
                        XSetWindowBorder(dpy, c->frame, c == focused ? theme.p_focus_border : theme.p_frame);
                        draw_frame(c);
                    }
                    set_wallpaper();
                }
                lua_close(L);
            }
        } break;
        case ACT_WALLPAPER: {
            size_t next = 0;
            for (size_t i=0; i<sizeof wallpaper_names/sizeof wallpaper_names[0]; ++i)
                if (!strcmp(config.wallpaper, wallpaper_names[i]) ||
                    (!strcmp(config.wallpaper, "default") && i == 0)) {
                    next = (i+1) % (sizeof wallpaper_names/sizeof wallpaper_names[0]);
                    break;
                }
            copystr(config.wallpaper, sizeof config.wallpaper, wallpaper_names[next]);
            set_wallpaper();
        } break;
        case ACT_EXIT: running=0; break;
    }
}
void reload_config(void) {
    hide_menu(); load_config();
    XSetWindowBorder(dpy, menu, theme.p_accent);

    draw_panel();
    for (Client *c=clients; c; c=c->next) {
        XSetWindowBorder(dpy, c->frame, c == focused ? theme.p_focus_border : theme.p_frame);
        draw_frame(c);
    }
    set_wallpaper();
}
