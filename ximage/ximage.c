/*************************************************************************************************************************************************
 * @filename: ximage.c
 * @author: EryangLi
 * @version: 1.0
 * @date: Oct-27-2023
 * @brief: ximagesink插件注册
 * @note: 
 * @history: 
 *          1. Date:
 *             Author:
 *             Modification:
 *      
 *          2. ..
 *************************************************************************************************************************************************
*/

#include "config.h"
#include "ximagesink.h"

/**
 * *************************************************************************************************************************************************
 * @name: plugin_init
 * @brief: 插件初始化
 * @param plugin: 插件对象地址
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
static gboolean
plugin_init (GstPlugin * plugin)
{
  if (g_getenv ("GST_XINITTHREADS"))
    XInitThreads ();

  return GST_ELEMENT_REGISTER (ximagesink, plugin);
}

/**
 * *************************************************************************************************************************************************
 * @name: GST_PLUGIN_DEFINE
 * @brief: 插件定义宏
 * @param major: major version number of the gstreamer-core that plugin was compiled for
 * @param minor: minor version number of the gstreamer-core that plugin was compiled for
 * @param name: short, but unique name of the plugin
 * @param description: information about the purpose of the plugin
 * @param init: function pointer to the plugin_init method with the signature of <code>static gboolean plugin_init (GstPlugin * plugin)</code>.
 * @param version: full version string (e.g. VERSION from config.h)
 * @param license: under which licence the package has been released, e.g. GPL, LGPL.
 * @param package: the package-name (e.g. PACKAGE_NAME from config.h)
 * @param origin: a description from where the package comes from (e.g. the homepage URL)
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    ximagesink,
    "X11 video output element based on standard Xlib calls",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
