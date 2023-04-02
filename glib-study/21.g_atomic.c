#include <glib.h>
#include <stdio.h>

int main()
{
    volatile gint atomic = 0;
    gint oldval = 0;

    gboolean success = g_atomic_int_compare_and_exchange(&atomic, oldval, 3);

    printf("success = %d, atomic = %d\n", success, atomic);

    const gchar *desktop_path = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
    g_print("Desktop path: %s\n", desktop_path);

    return 0;
}
