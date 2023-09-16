#include <stdio.h>
#include <gio/gio.h>

static void set_keybind(GSettings *gs);
static void get_keybind(GSettings *gs);

static void
set_keybind(GSettings *gs)
{
    gboolean ok;

    ok = g_settings_set_string(gs, "type", "网络摄像头");
    if (!ok) {
        g_warning("Failed to set name to 'test'");
    }

    ok = g_settings_set_string(gs, "link-param", "192.168.11.1");
    if (!ok) {
        g_warning("Failed to set shortcuts");
    }
}

static void
get_keybind(GSettings *gs)
{
    gchar *name, *link_param;

    name = g_settings_get_string(gs, "type");
    if (name) {
        g_print("Shortcut name: %s\n", name);
        g_free(name);
    }


    link_param = g_settings_get_string(gs, "link-param");
    if (link_param) {
        g_print("Shortcut desc: %s\n", link_param);
        g_free(link_param);
    }
}

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        g_print("Usage: %s <action> <keybind id>\n"
                "\taction: get or set\n"
                "\tkeybind id: a non-negative integer\n", argv[0]);
        return 0;
    }

    GSettings *gs;
    gchar buf[1024] = {0};

    if (sprintf(buf, "/org/vpf/camera/%s/", argv[2]) < 0) {
        g_error("Failed to combains path: %s\n", argv[2]);
        return -1;
    }
    gs = g_settings_new_with_path("org.vpf.camera", buf);
  
    if (g_strcmp0(argv[1], "set") == 0) {
        set_keybind(gs);
    } else if (g_strcmp0(argv[1], "get") == 0) {
        get_keybind(gs);
    } else {
        g_warning("invalid action: %s", argv[1]);
    }

    g_object_unref(gs);
    g_settings_sync();
    return 0;
}