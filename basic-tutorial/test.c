#include <gst/gst.h>
#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC (my_category);
#define GST_CAT_DEFAULT my_category

int main(int argc, char *argv[]) {
  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* gst初始化后初始 , 0日志字符表示无颜色输出， 1表示有颜色输出*/
  GST_DEBUG_CATEGORY_INIT (my_category, "my category", 0, "This is my very own");

  /* 通过以下函数，可以在自己的函数中添加debug调试信息 */
  GST_ERROR("My msg: %d", 0);
  GST_WARNING("My msg: %d", 1);
  GST_INFO("My msg: %d", 2);
  GST_DEBUG("My msg: %d", 3);

  return 0;
}