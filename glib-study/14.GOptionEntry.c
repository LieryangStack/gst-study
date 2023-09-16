// test.c

#include <glib.h>
#include <locale.h>

#define ERR_MSG_V(msg, ...) \
    g_print("** ERROR: <%s:%d>: " msg "\n", __func__, __LINE__, ##__VA_ARGS__)

static  gint repeats = 2;
static  gint max_size = 8;
static  gboolean verbose = FALSE;
static  gboolean beep = FALSE;
static  gboolean op_rand = FALSE;
static  gchar arg_data[32] = { "arg data" };

static  GOptionEntry entries[] =
{
  /* 全名 */ /* 简写 */ /*标志*/ /*数据类型*/ /* 存放数据缓冲区 */  /*描述*/      /* 变量描述 */
  { "repeats", 'r', 0, G_OPTION_ARG_INT, &repeats, "Average over N repetitions", "N" },
  { "max-size", 'm', 0, G_OPTION_ARG_INT, &max_size, "Test up to 2^M items", "M" },
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },
  { "beep", 'b', 0, G_OPTION_ARG_NONE, &beep, "Beep when done", NULL },
  { "rand", 0, 0, G_OPTION_ARG_NONE, &rand, "Randomize the data", NULL },
  { NULL }
};

static gchar *cfg_file = NULL;
static gchar *input_file = NULL;
static gchar *topic = NULL;
static gchar *conn_str = NULL;
static gchar *proto_lib = NULL;
static gint schema_type = 0;
static gboolean display_off = FALSE;

static GOptionEntry entries_1[] = {
  {"cfg-file", 'c', 0, G_OPTION_ARG_FILENAME, &cfg_file,
  "Set the adaptor config file. Optional if connection string has relevant  details.", NULL},
  {"input-file", 'i', 0, G_OPTION_ARG_FILENAME, &input_file,
   "Set the input H264 file", NULL},
  {"topic", 't', 0, G_OPTION_ARG_STRING, &topic,
   "Name of message topic. Optional if it is part of connection string or config file.", NULL},
  {"conn-str", 0, 0, G_OPTION_ARG_STRING, &conn_str,
   "Connection string of backend server. Optional if it is part of config file.", NULL},
  {"proto-lib", 'p', 0, G_OPTION_ARG_STRING, &proto_lib,
   "Absolute path of adaptor library", NULL},
  {"schema", 's', 0, G_OPTION_ARG_INT, &schema_type,
   "Type of message schema (0=Full, 1=minimal), default=0", NULL},
  {"no-display", 0, 0, G_OPTION_ARG_NONE, &display_off, "Disable display", NULL},
  {NULL}
};


int main (int  argc,  char  *argv[])
{
    GError *error = NULL;
    GOptionContext *context = NULL;
    GOptionGroup *group = NULL, *nv_group = NULL;

    // 创建一个新的选项上下文
    context = g_option_context_new("- test tree model performance" );
#if 0
    // 如果主要组不存在则创建主要组,向组添加entries并设置转换域
    g_option_context_add_main_entries(context, entries, NULL);

    //添加要在选项列表之前的--help输出中显示的字符串。 这通常是程序功能的摘要
    g_option_context_set_summary(context, "This is a glib demo" );
#else
    group = g_option_group_new ("abc", NULL, NULL, NULL, NULL);
    g_option_group_add_entries (group, entries);
    g_option_context_set_main_group (context, group);
                              /* 组名字 */ /*组描述信息*/    /*这个组help显示信息*/
    nv_group = g_option_group_new("nv", "nvidia-command", "Show NVIDIA Options", NULL, NULL);
    g_option_group_add_entries(nv_group, entries_1);
    g_option_context_add_group(context, nv_group);
#endif

    // 解析命令行参数，识别已添加到上下文的选项
    if  (!g_option_context_parse(context, &argc, &argv, &error))
    {
        ERR_MSG_V("%s", error->message);
        exit (1);
    }

    // 释放被解析的参数
    g_option_context_free(context);

    g_print("Now value is: repeats=%d, max_size=%d, verbose=%d, beep=%d\n",
            repeats, max_size, verbose, beep);

    return  0;
}
