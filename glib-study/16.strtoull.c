#include<glib.h>

int main(int argc, char *argv[]){

  gchar *endptr;
  gchar nptr[] = "-123abc1";
  gint64 ret = g_ascii_strtoll(nptr, &endptr, 10);

  g_print("ret = %ld\n", ret);
  g_print("str = %s\n", endptr);

  return 0;
}