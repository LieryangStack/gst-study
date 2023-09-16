/**
 * （1）GstQuery是向一个element或者pad询问一些信息的机制。
 * 在这个例子中我们会问pipeline是否支持跳转功能（实时流是不支持跳转功能的）
 * 如果支持跳转功能，那么在播放10s之后跳转到另一个位置。
 * （2）在前面的教程中，我们一旦建立pipeline并运行后，我们就是在等待ERROR或者EOS消息，
 * 这个例子中我们修改这部分，改成定时唤醒并查询pipeline当前播放的位置并在屏幕上显示出来。
*/
#include <gst/gst.h>

/* Structure to contain all our information, so we can pass it around */
typedef struct _CustomData
{
  GstElement *playbin;          /* Our one and only element */
  gboolean playing;             /* Are we in the PLAYING state? */
  gboolean terminate;           /* Should we terminate execution? */
  gboolean seek_enabled;        /* Is seeking enabled for this media? */
  gboolean seek_done;           /* Have we performed the seek already? */
  gint64 duration;              /* How long does this media last, 以纳秒为单位 */
} CustomData;

/* Forward definition of the message processing function */
static void handle_message (CustomData * data, GstMessage * msg);

int
main (int argc, char *argv[])
{
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;

  data.playing = FALSE;
  data.terminate = FALSE;
  data.seek_enabled = FALSE;
  data.seek_done = FALSE;
  data.duration = GST_CLOCK_TIME_NONE;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  data.playbin = gst_element_factory_make ("playbin", "playbin");

  if (!data.playbin) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Set the URI to play */
  /* 每个Element都有属性可以通过
   * g_object_set()设置属性
   * g_object_get()读属性
   */
  g_object_set (data.playbin, "uri",
      "file:///home/lieryang/Desktop/gst-study/sample-480p.webm",
      NULL);

  /* Start playing */
  /* 流管开始 */
  /* NULL -> READY -> PAUSED -> PLAYING */
  ret = gst_element_set_state (data.playbin, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.playbin);
    return -1;
  }

  /* Listen to the bus */
  /* 从流管上获得bus消息总线 */
  bus = gst_element_get_bus (data.playbin);
  do {
    /* 阻塞时间为 100 ms 
     * GstClokTime是以纳米为单位
     */
    msg = gst_bus_timed_pop_filtered (bus, 100 * GST_MSECOND,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS |
        GST_MESSAGE_DURATION);

    /* Parse message */
    if (msg != NULL) { /*msg有消息*/
      handle_message (&data, msg);
    } 
    else {
      /* We got no message, this means the timeout expired */
      if (data.playing) {
        gint64 current = -1;

        /* Query the current position of the stream */
        /* 查询流管当前时间，GST_FORMAT_TIME表示格式为时间格式 */
        if (!gst_element_query_position (data.playbin, GST_FORMAT_TIME,
                &current)) {
          g_printerr ("Could not query current position.\n");
        }

        /* If we didn't know it yet, query the stream duration */
        /* 查询流管总时间 */
        if (!GST_CLOCK_TIME_IS_VALID (data.duration)) {
          if (!gst_element_query_duration (data.playbin, GST_FORMAT_TIME,
                  &data.duration)) {
            g_printerr ("Could not query current duration.\n");
          }
        }

        /* Print current position and total duration */
        /* \r是将光标移动到本行开头，会覆盖掉本行内容 */
        g_print ("current = %ld Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
            current, GST_TIME_ARGS (current), GST_TIME_ARGS (data.duration));

        /* If seeking is enabled, we have not done it yet, and the time is right, seek */
        if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND) {
          g_print ("\nReached 10s, performing seek...\n");
          gst_element_seek_simple (data.playbin, GST_FORMAT_TIME,
              GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND);
          data.seek_done = TRUE;
        }
      }
    }
  } while (!data.terminate);

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (data.playbin, GST_STATE_NULL);
  gst_object_unref (data.playbin);
  return 0;
}

static void
handle_message (CustomData * data, GstMessage * msg)
{
  GError *err;
  gchar *debug_info;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (msg, &err, &debug_info);
      g_printerr ("Error received from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), err->message);
      g_printerr ("Debugging information: %s\n",
          debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_EOS:
      g_print ("\nEnd-Of-Stream reached.\n");
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_DURATION:
      /* The duration has changed, mark the current one as invalid */
      data->duration = GST_CLOCK_TIME_NONE;
      break;
    case GST_MESSAGE_STATE_CHANGED:{
      GstState old_state, new_state, pending_state;
      /* pending_state 即将发生的状态 */
      gst_message_parse_state_changed (msg, &old_state, &new_state,
          &pending_state);
      if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->playbin)) {
        g_print ("Pipeline state changed from %s to %s:\n",
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
        /* Remember whether we are in the PLAYING state or not */
        data->playing = (new_state == GST_STATE_PLAYING);
        /* 如果处于播放状态
         * 查询流媒体开始和结束时间
         */
        if (data->playing) {
          /* We just moved to PLAYING. Check if seeking is possible */
          GstQuery *query;
          gint64 start, end;
          query = gst_query_new_seeking (GST_FORMAT_TIME);
          if (gst_element_query (data->playbin, query)) {
            gst_query_parse_seeking (query, NULL, &data->seek_enabled, &start,
                &end);
            if (data->seek_enabled) {
              g_print ("Seeking is ENABLED from %" GST_TIME_FORMAT " to %"
                  GST_TIME_FORMAT "\n", GST_TIME_ARGS (start),
                  GST_TIME_ARGS (end));
            } else {
              g_print ("Seeking is DISABLED for this stream.\n");
            }
          } else {
            g_printerr ("Seeking query failed.");
          }
          gst_query_unref (query);
        }
      }
    }
      break;
    default:
      /* We should not reach here */
      g_printerr ("Unexpected message received.\n");
      break;
  }
  /*释放引用*/
  gst_message_unref (msg);
}
