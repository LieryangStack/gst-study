/**

 * Copyright (C), 2020-2099, Lieryng Tech. Co., Ltd.

 * FileName: 04_Memory_allocation.c

 * Author:  Lieryang   Version :  1.0    Date:Oct-26-2023

 * Description:      测试GstMemory对象的相关特性

 * Version:         // 版本信息

 * Function List:   // 主要函数及其功能

    1. -------

 * History:         // 历史修改记录

      <author>  <time>   <version >   <desc>


 */

#include <gst/gst.h>

int
main (int argc, char *argv[]) {

  gst_init (&argc, &argv);

  GstMemory *mem;
  GstMapInfo info;
  gint i;

  /* 参数allocator = NULL将使用默认的分配器 */
  mem = gst_allocator_alloc (NULL, 100, NULL);

  gst_memory_map (mem, &info, GST_MAP_WRITE);

  for (i = 0; i < info.size; i++)
    info.data[i] = i;

  /* release memory */
  gst_memory_unmap (mem, &info);

  gst_allocator_free (NULL, mem);

  return 0;
}