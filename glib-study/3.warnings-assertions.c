#include <glib.h>

int
main(int argc, char *argv[]){

  gint8 condition = 0;

  g_assert(condition);
  g_return_if_fail(condition);
  

  return 0;
}