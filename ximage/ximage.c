/**

 * Copyright (C), 2020-2099, Lieryng Tech. Co., Ltd.

 * FileName: ximage.c

 * Author:        Version :          Date:

 * Description:     // 模块描述

 * Version:         // 版本信息

 * Function List:   // 主要函数及其功能

    1. plugin_init

    2. GST_PLUGIN_DEFINE


 * History:         // 历史修改记录

      <author>  <time>   <version >   <desc>


 */

#include "config.h"

#include "ximagesink.h"


static gboolean
plugin_init (GstPlugin * plugin)
{
  if (g_getenv ("GST_XINITTHREADS"))
    XInitThreads ();

  return GST_ELEMENT_REGISTER (ximagesink, plugin);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    ximagesink,
    "X11 video output element based on standard Xlib calls",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
