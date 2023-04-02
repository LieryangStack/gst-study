#include<glib.h>

/**
 * FALSE，该事件源将被删除
 * TRUE，该事件源在没有更高优先级事件时，再次运行
*/
gboolean
count_down(gpointer data){
  static int count = 10;

  if(count < 1){
    g_print("count_down return FALSE\n");
    return FALSE;
  }
  g_print("count_down %4d\n", count--);
  return TRUE;
}

gboolean
cancel_fire(gpointer data){
  GMainLoop *loop = data;
  g_print("cancel_fire() quit \n");
  g_main_loop_quit(loop);

  return FALSE;
}

gboolean
say_idle(gpointer data){
  g_print("say_idle() \n");
  return FALSE;
}

int
main(int argc, char *argv[]){

  /* 每个事件源都会关联一个GMainContext，一个线程只能运行一个GMainContext
   * 但是其他线程中能够对事件源进行添加和删除操作
   */
  GMainContext *context;

  GMainLoop *loop = g_main_loop_new(NULL, FALSE);

  /*添加超时事件源*/
  g_timeout_add(1000, count_down, NULL);
  g_timeout_add(8000, cancel_fire, loop);

  /*添加一个空闲函数*/
  g_idle_add(say_idle, NULL);
  g_main_loop_run(loop);

  g_main_loop_unref(loop);

  return 0;
}