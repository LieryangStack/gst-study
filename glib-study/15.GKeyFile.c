#include <gst/gst.h>

int
main(int argc, char *argv[]){

  GKeyFile *config;
  gchar *str;
  gsize str_size;
  gchar *content;
  
  config = g_key_file_new();
  g_key_file_load_from_file(config, argv[1], G_KEY_FILE_NONE, NULL);

  str = g_key_file_get_string(config, "Family", "name", NULL);
  g_print("name = %s\n", str);
  
  g_key_file_set_string(config, "Family", "name", "Li");
  

  content = g_key_file_to_data(config, &str_size, NULL);
  g_file_set_contents(argv[1], content, str_size, NULL);

  g_key_file_free(config);
  g_free(content);

  return 0;
}