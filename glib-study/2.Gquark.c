#include <glib.h>

int
main(int argc, char *argv[]){

  GQuark quark;
  const gchar *name = "EryangLi";
  gint32 flag = 0;

  quark = g_quark_from_static_string(name);
  g_print("%s has been Quark %d\n",name, quark);
  g_print("%s pointer %p\n%s Quark pointer %p\n", name, name, name, g_quark_to_string(quark));

  flag = g_quark_try_string(name);

  g_print("flag = %d\n", flag);

  return 0;
}