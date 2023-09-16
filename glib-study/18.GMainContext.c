#include<glib.h>

static gboolean
test (gpointer user_data) {
  g_print ("%s\n", __func__);

  return FALSE;
}

GMainContext *context;

static void *
thread_test2 (gpointer user_data) { 
  GMainContext *context1 = g_main_context_new ();
  GMainLoop *loop = g_main_loop_new (context1, FALSE);
  g_usleep (2 * G_USEC_PER_SEC);

  g_main_context_push_thread_default (context1);
  g_message ("2:g_main_context_new1:%p\n", context1);
  context1 = g_main_context_get_thread_default ();
  g_message ("2:g_main_context_get_thread_default:%p\n", context1);

  g_main_loop_run (loop);
}

static void *
thread_test1 (gpointer user_data) {
  GMainContext *context1 = g_main_context_new ();
  GMainLoop *loop = g_main_loop_new (context1, FALSE);
  g_usleep (2 * G_USEC_PER_SEC);

  g_main_context_push_thread_default (context1);
  g_message ("1:g_main_context_new1:%p\n", context1);
  context1 = g_main_context_get_thread_default ();
  g_message ("1:g_main_context_get_thread_default:%p\n", context1);

  g_main_loop_run (loop);

  // GMainContext *context2 = g_main_context_new ();
  // g_main_context_push_thread_default (context2);
  // g_message ("g_main_context_new2:%p\n", context2);
  // context2 = g_main_context_get_thread_default ();
  // g_message ("g_main_context_get_thread_default:%p\n", context2);

  // g_main_context_push_thread_default (context1);
  // context2 = g_main_context_get_thread_default ();
  // g_message ("g_main_context_push_thread_default After\ng_main_context_push_thread_default:%p\n", context2);

  // GMainContext *context = g_main_context_new ();
  // g_main_context_push_thread_default (context);
  // g_message ("g_main_context_new:%p\n", context);
  // g_main_context_pop_thread_default(context);
  // context = g_main_context_get_thread_default ();
  // g_message ("g_main_context_get_thread_default:%p\n", context);
  // context = g_main_context_default ();
  // g_message ("g_main_context_default:%p\n", context);
}

int
main(int argc, char *argv[]){
  GMainLoop *loop;
  gboolean acquired_context;
  
  loop = g_main_loop_new (NULL, FALSE);
  context = g_main_loop_get_context(loop);
  g_message ("g_main_loop_get_context:%p\n", context);
  context = g_main_loop_get_context(loop);
  g_message ("g_main_loop_get_context:%p\n", context);
  

  context = g_main_context_get_thread_default ();
  g_message ("g_main_context_get_thread_default:%p\n", context);
  context = g_main_context_default ();
  g_message ("g_main_context_default:%p\n", context);
  acquired_context = g_main_context_acquire (NULL);
  if (!acquired_context) {
    g_critical ("g_application_run() cannot acquire the default \
      ain context because it is already acquired by another thread!");
  }
  //g_idle_add (test, NULL);

  g_thread_new ("app.test1", thread_test1, NULL);
  g_thread_new ("app.test2", thread_test2, NULL);
  
  while (1) {
    g_print ("%s\n", __func__);
    g_main_context_iteration (context, TRUE);
  }
  return 0;
}