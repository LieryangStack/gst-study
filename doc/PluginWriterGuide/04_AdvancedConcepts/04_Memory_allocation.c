/**

 * Copyright (C), 2020-2099, Lieryng Tech. Co., Ltd.

 * FileName: 04_Memory_allocation.c

 * Author:  Lieryang   Version :  1.0    Date:Oct-26-2023

 * Description:      1.测试GstMemory对象的相关特性
                     2.测试GstBuffer对象的相关特性
                     3.
 * Version:         // 版本信息

 * Function List:   // 主要函数及其功能

    1. -------

 * History:         // 历史修改记录

      <author>  <time>   <version >   <desc>


 */

#include <gst/gst.h>
#include <gst/video/gstvideometa.h>

int
main (int argc, char *argv[]) {

  gst_init (&argc, &argv);

  GstMemory *mem1, *mem2;
  GstMapInfo info;
  gint i;

  /* 参数allocator = NULL将使用默认的分配器 */
  mem1 = gst_allocator_alloc (NULL, 100, NULL);
  mem2 = gst_allocator_alloc (NULL, 50, NULL);

  g_print ("Memory type: %s\n", mem1->allocator->mem_type);

  g_print ("mem->maxsize = %ld\n", mem1->maxsize);
  g_print ("mem->size = %ld\n", mem1->size);
  g_print ("mem->offset = %ld\n", mem1->offset);
  /* 暂时没有了解对齐的原因 */
  g_print ("mem->align = %ld\n", mem1->align);
  g_print ("mem->parent = %p\n", mem1->parent);

  gst_memory_map (mem1, &info, GST_MAP_WRITE);

  for (i = 0; i < info.size; i++)
    info.data[i] = i;

  /* release memory */
  gst_memory_unmap (mem1, &info);

  /************测试GstBuffer***************/
  GstBuffer *buffer = gst_buffer_new ();
  gst_buffer_append_memory (buffer, mem1);
  gst_buffer_append_memory (buffer, mem2);

  /* 这里会合并所有GstMemory */
  gst_buffer_map (buffer, &info, GST_MAP_WRITE);
  g_print ("info.maxsize = %ld\n", info.maxsize);
  g_print ("info.size = %ld\n", info.size);

  // memset (info.data, 0xff, info.size);
  gst_buffer_unmap (buffer, &info);

  /**************GstMeta****************/
  
  /* buffer points to a video frame, add some cropping metadata */
  GstVideoCropMeta *meta = gst_buffer_add_video_crop_meta (buffer);
  /* configure the cropping metadata */
  meta->x = 8;
  meta->y = 8;
  meta->width = 120;
  meta->height = 80;
  
  gst_buffer_unref (buffer);

  gst_allocator_free (mem1->allocator, mem1);
  gst_allocator_free (mem2->allocator, mem2);

  g_print ("mem1->mini_object.refcount = %d\n", mem1->mini_object.refcount);
  g_print ("mem2->mini_object.refcount = %d\n", mem2->mini_object.refcount);



  return 0;
}