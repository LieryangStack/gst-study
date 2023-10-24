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
