/*************************************************************************************************************************************************
 * @filename: ximagepool.c
 * @author: EryangLi
 * @version: 1.0
 * @date: Oct-27-2023
 * @brief:  1. 定义了一个GstXImageMemoryAllocator对象，该对象只在本源文件中使用
 *             GstXImageMemoryAllocator继承于抽象类型GstAllocator（GstObject类对象）
 * 
 *          2.GstXImageBufferPool对象的相关实现
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

/* Object header */
#include "ximagesink.h"

/* Debugging category */
#include <gst/gstinfo.h>

/* Helper functions */
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

GST_DEBUG_CATEGORY (gst_debug_x_image_pool);
#define GST_CAT_DEFAULT gst_debug_x_image_pool

/* X11 stuff */
static gboolean error_caught = FALSE;

/******************************GstXImageMemoryAllocator对象相关************START****************************/

/***
 * 定义对象GstXImageMemoryAllocator，因为实例结构体和类结构体没有添加新的变量
 * 所以这里使用别名，就不需要再定义 struct _GstAllocator{} 和 struct _GstAllocatorClass{}
*/
typedef GstAllocator GstXImageMemoryAllocator;
typedef GstAllocatorClass GstXImageMemoryAllocatorClass;

/* 上述做法相当于以下四行 */

// typedef struct _GstXImageMemoryAllocator GstXImageMemoryAllocator;
// typedef struct _GstXImageMemoryAllocatorClass GstXImageMemoryAllocatorClass;

// struct _GstXImageMemoryAllocator {GstAllocator parent;};
// struct _GstXImageMemoryAllocatorClass {GstAllocatorClass parent_class;};

GType ximage_memory_allocator_get_type (void);

#define GST_XIMAGE_ALLOCATOR_NAME "ximage"
#define GST_TYPE_XIMAGE_MEMORY_ALLOCATOR   (ximage_memory_allocator_get_type())
#define GST_IS_XIMAGE_MEMORY_ALLOCATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_XIMAGE_MEMORY_ALLOCATOR))

G_DEFINE_TYPE (GstXImageMemoryAllocator, ximage_memory_allocator,
    GST_TYPE_ALLOCATOR);

/**
 * *************************************************************************************************************************************************
 * @brief: 暂时不知道为什么不使用该函数分配内存
 * @param allocator:
 * @param size: 
 * @param params: 
 * @return:
 * @note: GstAllocatorClass *allocator_class->alloc = gst_ximage_memory_alloc; 
 *        抽象类GstAllocator虚函数实现
 * *************************************************************************************************************************************************
*/
static GstMemory *
gst_ximage_memory_alloc (GstAllocator * allocator, gsize size,
    GstAllocationParams * params) {
  return NULL;
}

/**
 * *************************************************************************************************************************************************
 * @brief: 1.仅仅释放GstXImageMemory结构体占用的内存
 *         2.XDestroyImage (mem->ximage);
 *           GstXImageMemory用户实际使用的内存并不在这里释放
 * @param allocator: 分配器地址
 * @param gmem: 本质是GstXImageMemory型对象
 * @return:
 * @note: GstAllocatorClass *allocator_class->free = gst_ximage_memory_free; 
 *        抽象类GstAllocator虚函数实现
 * *************************************************************************************************************************************************
*/
static void
gst_ximage_memory_free (GstAllocator * allocator, GstMemory * gmem)
{
  GstXImageMemory *mem = (GstXImageMemory *) gmem;
  GstXImageSink *ximagesink;

  /* 为什么GstMemory对象的成员parent有指向，就不去执行以下操作？？？？？？ */
  if (gmem->parent)
    goto sub_mem;

  ximagesink = mem->sink;

  GST_DEBUG_OBJECT (ximagesink, "free memory %p", mem);

  /* 上锁，保证其他线程不会操作XContext */
  GST_OBJECT_LOCK (ximagesink);

  /* 当状态处于NULL之后，可能有一些内存需要被销毁 */
  if (ximagesink->xcontext == NULL) {
    GST_DEBUG_OBJECT (ximagesink, "Destroying XImage after XContext");
#ifdef HAVE_XSHM /* x11扩展，内存共享机制，暂时没有了解 */
    /* 如果XContext上下文 == NULL，我们应该讲共享内存段与当前进程分离 */
    if (mem->SHMInfo.shmaddr != ((void *) -1)) {
      shmdt (mem->SHMInfo.shmaddr);
    }
#endif
    goto beach;
  }

  /* 目前xcontext不处于 NULL 状态，销毁mem->ximage */
  g_mutex_lock (&ximagesink->x_lock);

#ifdef HAVE_XSHM /* x11扩展，内存共享机制，暂时没有了解 */
  if (ximagesink->xcontext->use_xshm) {
    if (mem->SHMInfo.shmaddr != ((void *) -1)) {
      GST_DEBUG_OBJECT (ximagesink, "XServer ShmDetaching from 0x%x id 0x%lx",
          mem->SHMInfo.shmid, mem->SHMInfo.shmseg);
      XShmDetach (ximagesink->xcontext->disp, &mem->SHMInfo);
      XSync (ximagesink->xcontext->disp, FALSE);
      shmdt (mem->SHMInfo.shmaddr);
      mem->SHMInfo.shmaddr = (void *) -1;
    }
    if (mem->ximage)
      XDestroyImage (mem->ximage);
  } else
#endif /* HAVE_XSHM */
  {
    if (mem->ximage) {
      XDestroyImage (mem->ximage);
    }
  }

  XSync (ximagesink->xcontext->disp, FALSE);

  g_mutex_unlock (&ximagesink->x_lock);

beach:
  GST_OBJECT_UNLOCK (ximagesink);

  gst_object_unref (mem->sink);

sub_mem:
  g_slice_free (GstXImageMemory, mem); /* 仅仅释放GstXImageMemory结构体占用的内存 */
}

/**
 * *************************************************************************************************************************************************
 * @brief: 获取到真正用户需要的数据地址（在这里就是图像数据的实际地址）
 * @param mem: 
 * @param maxsize: 
 * @param flags: 
 * @return:
 * @note:实例成员函数指针  mem_map
 *       GstAllocator *alloc->mem_map = (GstMemoryMapFunction) ximage_memory_map;
 * *************************************************************************************************************************************************
*/
static gpointer
ximage_memory_map (GstXImageMemory * mem, gsize maxsize, GstMapFlags flags)
{
  return mem->ximage->data + mem->parent.offset;
}

/**
 * *************************************************************************************************************************************************
 * @brief: 不清楚具体作用？？？？ 为什么只返回TRUE
 * @param mem: 
 * @return:
 * @note:实例成员函数指针  mem_unmap 
 *       GstAllocator *alloc->mem_unmap = (GstMemoryUnmapFunction) ximage_memory_unmap;
 * *************************************************************************************************************************************************
*/
static gboolean
ximage_memory_unmap (GstXImageMemory * mem)
{
  return TRUE;
}

/**
 * *************************************************************************************************************************************************
 * @brief: 暂时还没有了解具体用途！！！！
 * @param mem: 
 * @param offset: 
 * @param size: 
 * @return:
 * @note: 实例成员函数指针  mem_share 
 *        GstAllocator *alloc->mem_share = (GstMemoryShareFunction) ximage_memory_share;
 * *************************************************************************************************************************************************
*/
static GstXImageMemory *
ximage_memory_share (GstXImageMemory * mem, gssize offset, gsize size)
{
  GstXImageMemory *sub;
  GstMemory *parent;

  /* We can only share the complete memory */
  if (offset != 0)
    return NULL;
  if (size != -1 && size != mem->size)
    return NULL;

  /* find the real parent */
  if ((parent = mem->parent.parent) == NULL)
    parent = (GstMemory *) mem;

  if (size == -1)
    size = mem->parent.size - offset;

  /* the shared memory is always readonly */
  sub = g_slice_new (GstXImageMemory);

  gst_memory_init (GST_MEMORY_CAST (sub), GST_MINI_OBJECT_FLAGS (parent) |
      GST_MINI_OBJECT_FLAG_LOCK_READONLY, mem->parent.allocator,
      &mem->parent, mem->parent.maxsize, mem->parent.align,
      mem->parent.offset + offset, size);
  sub->sink = mem->sink;
  sub->ximage = mem->ximage;
#ifdef HAVE_XSHM
  sub->SHMInfo = mem->SHMInfo;
#endif
  sub->x = mem->x;
  sub->y = mem->y;
  sub->width = mem->width;
  sub->height = mem->height;

  return sub;
}


/**
 * *************************************************************************************************************************************************
 * @brief: GstXImageMemoryAllocator类初始化函数
 * @param klass:
 * @return:
 * @note: 
 * *************************************************************************************************************************************************
*/
static void
ximage_memory_allocator_class_init (GstXImageMemoryAllocatorClass * klass)
{
  GstAllocatorClass *allocator_class;

  allocator_class = (GstAllocatorClass *) klass;

  allocator_class->alloc = gst_ximage_memory_alloc;
  allocator_class->free = gst_ximage_memory_free;
}

/**
 * *************************************************************************************************************************************************
 * @brief: GstXImageMemoryAllocator实例初始化函数
 * @param allocator: 
 * @return:
 * @note: 
 * *************************************************************************************************************************************************
*/
static void
ximage_memory_allocator_init (GstXImageMemoryAllocator * allocator)
{
  GstAllocator *alloc = GST_ALLOCATOR_CAST (allocator);

  alloc->mem_type = GST_XIMAGE_ALLOCATOR_NAME;
  alloc->mem_map = (GstMemoryMapFunction) ximage_memory_map;
  alloc->mem_unmap = (GstMemoryUnmapFunction) ximage_memory_unmap;
  alloc->mem_share = (GstMemoryShareFunction) ximage_memory_share;

  /* fallback copy and is_span */

  GST_OBJECT_FLAG_SET (allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);
}

/***************************************************GstXImageMemoryAllocator********************************END**************************************/


/**************************************************GstXImageBufferPool相关***********************************START************************************/

#define gst_ximage_buffer_pool_parent_class parent_class
G_DEFINE_TYPE (GstXImageBufferPool, gst_ximage_buffer_pool,
    GST_TYPE_BUFFER_POOL);

/**
 * *************************************************************************************************************************************************
 * @brief: X11窗口发生错误会被调用
 * @param display:  
 * @param xevent: 
 * @return: 
 * @calledby: ximage_memory_alloc
 *            gst_x_image_sink_check_xshm_calls
 * @note:
 * *************************************************************************************************************************************************
*/
static int
gst_ximagesink_handle_xerror (Display * display, XErrorEvent * xevent)
{
  char error_msg[1024];

  XGetErrorText (display, xevent->error_code, error_msg, 1024);
  GST_DEBUG ("ximagesink triggered an XError. error: %s", error_msg);
  error_caught = TRUE;
  return 0;
}

/**
 * *************************************************************************************************************************************************
 * @brief: 1.仅仅申请了GstXImageMemory结构体需要的内存 （用户需要的内存地址为mem->ximage->data）
 *         2.
 * @param xpool: 
 * @called_by: 被ximage_buffer_pool_alloc调用 (它是GstXImageBufferPoolClass->alloc虚函数的实现)
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
static GstMemory *
ximage_memory_alloc (GstXImageBufferPool * xpool) {

  GstXImageSink *ximagesink;
  int (*handler) (Display *, XErrorEvent *);
  gboolean success = FALSE;
  GstXContext *xcontext;
  gint width, height, align, offset;
  GstXImageMemory *mem;
  
  ximagesink = xpool->sink;
  xcontext = ximagesink->xcontext;

  width = xpool->padded_width;
  height = xpool->padded_height;

  /* 1.GstXImageMemory对象申请了内存 */
  mem = g_slice_new (GstXImageMemory);

#ifdef HAVE_XSHM
  mem->SHMInfo.shmaddr = ((void *) -1);
  mem->SHMInfo.shmid = -1;
#endif
  mem->x = xpool->align.padding_left;
  mem->y = xpool->align.padding_top;
  mem->width = GST_VIDEO_INFO_WIDTH (&xpool->info);
  mem->height = GST_VIDEO_INFO_HEIGHT (&xpool->info);
  mem->sink = gst_object_ref (ximagesink);

  GST_DEBUG_OBJECT (ximagesink, "creating image %p (%dx%d)", mem,
      width, height);

  g_mutex_lock (&ximagesink->x_lock);

  /* 设定一个错误处理者，捕获失败 */
  error_caught = FALSE;
  handler = XSetErrorHandler (gst_ximagesink_handle_xerror);

#ifdef HAVE_XSHM
  if (xcontext->use_xshm) {
    mem->ximage = XShmCreateImage (xcontext->disp,
        xcontext->visual,
        xcontext->depth, ZPixmap, NULL, &mem->SHMInfo, width, height);
    if (!mem->ximage || error_caught) {
      g_mutex_unlock (&ximagesink->x_lock);

      /* Reset error flag */
      error_caught = FALSE;

      /* Push a warning */
      GST_ELEMENT_WARNING (ximagesink, RESOURCE, WRITE,
          ("Failed to create output image buffer of %dx%d pixels",
              width, height),
          ("could not XShmCreateImage a %dx%d image", width, height));

      /* Retry without XShm */
      ximagesink->xcontext->use_xshm = FALSE;

      /* Hold X mutex again to try without XShm */
      g_mutex_lock (&ximagesink->x_lock);

      goto no_xshm;
    }

    /* we have to use the returned bytes_per_line for our shm size */
    mem->size = mem->ximage->bytes_per_line * mem->ximage->height;
    GST_LOG_OBJECT (ximagesink,
        "XShm image size is %" G_GSIZE_FORMAT ", width %d, stride %d",
        mem->size, width, mem->ximage->bytes_per_line);

    /* get shared memory */
    align = 0;
    mem->SHMInfo.shmid =
        shmget (IPC_PRIVATE, mem->size + align, IPC_CREAT | 0777);
    if (mem->SHMInfo.shmid == -1)
      goto shmget_failed;

    /* attach */
    mem->SHMInfo.shmaddr = shmat (mem->SHMInfo.shmid, NULL, 0);
    if (mem->SHMInfo.shmaddr == ((void *) -1))
      goto shmat_failed;

    /* now we can set up the image data */
    mem->ximage->data = mem->SHMInfo.shmaddr;
    mem->SHMInfo.readOnly = FALSE;

    if (XShmAttach (xcontext->disp, &mem->SHMInfo) == 0)
      goto xattach_failed;

    XSync (xcontext->disp, FALSE);

    /* Now that everyone has attached, we can delete the shared memory segment.
     * This way, it will be deleted as soon as we detach later, and not
     * leaked if we crash. */
    shmctl (mem->SHMInfo.shmid, IPC_RMID, NULL);

    GST_DEBUG_OBJECT (ximagesink, "XServer ShmAttached to 0x%x, id 0x%lx",
        mem->SHMInfo.shmid, mem->SHMInfo.shmseg);
  } else
  no_xshm:
#endif /* HAVE_XSHM */
  {
    guint allocsize;
    
    mem->ximage = XCreateImage (xcontext->disp,
        xcontext->visual,
        xcontext->depth, ZPixmap, 0, NULL, width, height, xcontext->bpp, 0);
    if (!mem->ximage || error_caught)
      goto create_failed;

    /* upstream will assume that rowstrides are multiples of 4, but this
     * doesn't always seem to be the case with XCreateImage() */
    if ((mem->ximage->bytes_per_line % 4) != 0) {
      GST_WARNING_OBJECT (ximagesink, "returned stride not a multiple of 4 as "
          "usually assumed");
    }

    /* we have to use the returned bytes_per_line for our image size */
    mem->size = mem->ximage->bytes_per_line * mem->ximage->height;

    /* alloc a bit more for unexpected strides to avoid crashes upstream.
     * FIXME: if we get an unrounded stride, the image will be displayed
     * distorted, since all upstream elements assume a rounded stride */
    allocsize =
        GST_ROUND_UP_4 (mem->ximage->bytes_per_line) * mem->ximage->height;

    /* we want 16 byte aligned memory, g_malloc may only give 8 */
    align = 15;
    mem->ximage->data = g_malloc (allocsize + align);
    GST_LOG_OBJECT (ximagesink,
        "non-XShm image size is %" G_GSIZE_FORMAT " (allocated: %u), width %d, "
        "stride %d", mem->size, allocsize, width, mem->ximage->bytes_per_line);

    XSync (xcontext->disp, FALSE);
  }

  if ((offset = ((guintptr) mem->ximage->data & align)))
    offset = (align + 1) - offset;

  GST_DEBUG_OBJECT (ximagesink, "memory %p, align %d, offset %d",
      mem->ximage->data, align, offset);

  /* Reset error handler */
  error_caught = FALSE;
  XSetErrorHandler (handler);

  gst_memory_init (GST_MEMORY_CAST (mem), GST_MEMORY_FLAG_NO_SHARE,
      xpool->allocator, NULL, mem->size + align, align, offset, mem->size);

  g_mutex_unlock (&ximagesink->x_lock);

  success = TRUE;
  
beach:
  if (!success) {
    g_slice_free (GstXImageMemory, mem);
    mem = NULL;
  }

  return GST_MEMORY_CAST (mem);

  /* ERRORS */
create_failed:
  {
    g_mutex_unlock (&ximagesink->x_lock);
    /* Reset error handler */
    error_caught = FALSE;
    XSetErrorHandler (handler);
    /* Push an error */
    GST_ELEMENT_ERROR (ximagesink, RESOURCE, WRITE,
        ("Failed to create output image buffer of %dx%d pixels",
            width, height),
        ("could not XShmCreateImage a %dx%d image", width, height));
    goto beach;
  }
#ifdef HAVE_XSHM
shmget_failed:
  {
    g_mutex_unlock (&ximagesink->x_lock);
    GST_ELEMENT_ERROR (ximagesink, RESOURCE, WRITE,
        ("Failed to create output image buffer of %dx%d pixels",
            width, height),
        ("could not get shared memory of %" G_GSIZE_FORMAT " bytes",
            mem->size));
    goto beach;
  }
shmat_failed:
  {
    g_mutex_unlock (&ximagesink->x_lock);
    GST_ELEMENT_ERROR (ximagesink, RESOURCE, WRITE,
        ("Failed to create output image buffer of %dx%d pixels",
            width, height), ("Failed to shmat: %s", g_strerror (errno)));
    /* Clean up the shared memory segment */
    shmctl (mem->SHMInfo.shmid, IPC_RMID, NULL);
    goto beach;
  }
xattach_failed:
  {
    /* Clean up the shared memory segment */
    shmctl (mem->SHMInfo.shmid, IPC_RMID, NULL);
    g_mutex_unlock (&ximagesink->x_lock);

    GST_ELEMENT_ERROR (ximagesink, RESOURCE, WRITE,
        ("Failed to create output image buffer of %dx%d pixels",
            width, height), ("Failed to XShmAttach"));
    goto beach;
  }
#endif
}

#ifdef HAVE_XSHM
/* This function checks that it is actually really possible to create an image
   using XShm */



/**
 * *************************************************************************************************************************************************
 * @brief: 
 * @param xpool: 
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
static const gchar **
ximage_buffer_pool_get_options (GstBufferPool * pool)
{
  static const gchar *options[] = { GST_BUFFER_POOL_OPTION_VIDEO_META,
    GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT, NULL
  };
  
  return options;
}


/**
 * *************************************************************************************************************************************************
 * @brief: 
 * @param xpool: 
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
static gboolean
ximage_buffer_pool_set_config (GstBufferPool * pool, GstStructure * config)
{
  GstXImageBufferPool *xpool = GST_XIMAGE_BUFFER_POOL_CAST (pool);
  GstVideoInfo info;
  GstCaps *caps;
  guint size, min_buffers, max_buffers;

  if (!gst_buffer_pool_config_get_params (config, &caps, &size, &min_buffers,
          &max_buffers))
    goto wrong_config;

  if (caps == NULL)
    goto no_caps;

  /* now parse the caps from the config */
  if (!gst_video_info_from_caps (&info, caps))
    goto wrong_caps;

  GST_LOG_OBJECT (pool, "%dx%d, caps %" GST_PTR_FORMAT, info.width, info.height,
      caps);

  /* keep track of the width and height and caps */
  if (xpool->caps)
    gst_caps_unref (xpool->caps);
  xpool->caps = gst_caps_ref (caps);

  /* check for the configured metadata */
  xpool->add_metavideo =
      gst_buffer_pool_config_has_option (config,
      GST_BUFFER_POOL_OPTION_VIDEO_META);

  /* parse extra alignment info */
  xpool->need_alignment = gst_buffer_pool_config_has_option (config,
      GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT);

  if (xpool->need_alignment) {
    gst_buffer_pool_config_get_video_alignment (config, &xpool->align);

    GST_LOG_OBJECT (pool, "padding %u-%ux%u-%u", xpool->align.padding_top,
        xpool->align.padding_left, xpool->align.padding_left,
        xpool->align.padding_bottom);

    /* do padding and alignment */
    gst_video_info_align (&info, &xpool->align);

    gst_buffer_pool_config_set_video_alignment (config, &xpool->align);

    /* we need the video metadata too now */
    xpool->add_metavideo = TRUE;
  } else {
    gst_video_alignment_reset (&xpool->align);
  }

  /* add the padding */
  xpool->padded_width =
      GST_VIDEO_INFO_WIDTH (&info) + xpool->align.padding_left +
      xpool->align.padding_right;
  xpool->padded_height =
      GST_VIDEO_INFO_HEIGHT (&info) + xpool->align.padding_top +
      xpool->align.padding_bottom;

  xpool->info = info;

  gst_buffer_pool_config_set_params (config, caps, info.size, min_buffers,
      max_buffers);

  return GST_BUFFER_POOL_CLASS (parent_class)->set_config (pool, config);

  /* ERRORS */
wrong_config:
  {
    GST_WARNING_OBJECT (pool, "invalid config");
    return FALSE;
  }
no_caps:
  {
    GST_WARNING_OBJECT (pool, "no caps in config");
    return FALSE;
  }
wrong_caps:
  {
    GST_WARNING_OBJECT (pool,
        "failed getting geometry from caps %" GST_PTR_FORMAT, caps);
    return FALSE;
  }
}

/* This function handles GstXImageBuffer creation depending on XShm availability */

/**
 * *************************************************************************************************************************************************
 * @brief:   GstXImageBufferClass->alloc虚函数实现
 *           gstbufferpool_class->alloc_buffer = ximage_buffer_pool_alloc
 *         
 *         1.创建了一个GstBuffer对象 ximage = gst_buffer_new ();
 *         2.申请了GstMemory对象内存（仅仅GstMemory对象），用户实际使用的data并没申请，调用ximage_memory_alloc(xpool)
 *         3.如果需要添加视频格式、宽度、高度等数据到GstBuffer， gst_buffer_add_video_meta_full
 * @param xpool: 
 * @param buffer: 
 * @param params: 
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
static GstFlowReturn
ximage_buffer_pool_alloc (GstBufferPool * pool, GstBuffer ** buffer,
    GstBufferPoolAcquireParams * params) {
  GstXImageBufferPool *xpool = GST_XIMAGE_BUFFER_POOL_CAST (pool);
  GstVideoInfo *info;
  GstBuffer *ximage;
  GstMemory *mem;

  info = &xpool->info;

  /* 1.创建了一个GstBuffer对象 */
  ximage = gst_buffer_new ();
  /* 2.申请结构体GstXImageMemory内存 */
  mem = ximage_memory_alloc (xpool);
  if (mem == NULL) {
    gst_buffer_unref (ximage);
    goto no_buffer;
  }
  /* 上一步申请的GstXImageMemory添加到GstBuffer */
  gst_buffer_append_memory (ximage, mem);

  /* 3.添加视频格式、宽度、高度等数据到GstBuffer */
  if (xpool->add_metavideo) {
    GstVideoMeta *meta;

    GST_DEBUG_OBJECT (pool, "adding GstVideoMeta");
    /* these are just the defaults for now */
    meta = gst_buffer_add_video_meta_full (ximage, GST_VIDEO_FRAME_FLAG_NONE,
        GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_WIDTH (info),
        GST_VIDEO_INFO_HEIGHT (info), GST_VIDEO_INFO_N_PLANES (info),
        info->offset, info->stride);

    gst_video_meta_set_alignment (meta, xpool->align);
  }
  *buffer = ximage;

  return GST_FLOW_OK;

  /* ERROR */
no_buffer:
  {
    GST_WARNING_OBJECT (pool, "can't create image");
    return GST_FLOW_ERROR;
  }
}

/**
 * *************************************************************************************************************************************************
 * @brief: 
 * @param xpool: 
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
static void
gst_ximage_buffer_pool_finalize (GObject * object)
{
  GstXImageBufferPool *pool = GST_XIMAGE_BUFFER_POOL_CAST (object);

  GST_LOG_OBJECT (pool, "finalize XImage buffer pool %p", pool);

  if (pool->caps)
    gst_caps_unref (pool->caps);
  gst_object_unref (pool->sink);
  gst_object_unref (pool->allocator);

  G_OBJECT_CLASS (gst_ximage_buffer_pool_parent_class)->finalize (object);
}


/**
 * *************************************************************************************************************************************************
 * @brief: 
 * @param xpool: 
 * @return:
 * @note: 1.gobject_class->finalize = gst_ximage_buffer_pool_finalize
 *        2.gstbufferpool_class->get_options = ximage_buffer_pool_get_options;
 *        3.gstbufferpool_class->set_config = ximage_buffer_pool_set_config;
 *        4.gstbufferpool_class->alloc_buffer = ximage_buffer_pool_alloc;
 * *************************************************************************************************************************************************
*/
static void
gst_ximage_buffer_pool_class_init (GstXImageBufferPoolClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBufferPoolClass *gstbufferpool_class = (GstBufferPoolClass *) klass;

  gobject_class->finalize = gst_ximage_buffer_pool_finalize;

  gstbufferpool_class->get_options = ximage_buffer_pool_get_options;
  gstbufferpool_class->set_config = ximage_buffer_pool_set_config;
  gstbufferpool_class->alloc_buffer = ximage_buffer_pool_alloc;
}

/**
 * *************************************************************************************************************************************************
 * @brief: 
 * @param xpool: 
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
static void
gst_ximage_buffer_pool_init (GstXImageBufferPool * pool)
{
  /* nothing to do here */
}


/**
 * *************************************************************************************************************************************************
 * @brief: 
 * @param xpool: 
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
GstBufferPool *
gst_ximage_buffer_pool_new (GstXImageSink * ximagesink)
{
  GstXImageBufferPool *pool;

  g_return_val_if_fail (GST_IS_X_IMAGE_SINK (ximagesink), NULL);

  pool = g_object_new (GST_TYPE_XIMAGE_BUFFER_POOL, NULL);
  gst_object_ref_sink (pool);
  pool->sink = gst_object_ref (ximagesink);
  pool->allocator = g_object_new (GST_TYPE_XIMAGE_MEMORY_ALLOCATOR, NULL);
  gst_object_ref_sink (pool->allocator);

  GST_LOG_OBJECT (pool, "new XImage buffer pool %p", pool);

  return GST_BUFFER_POOL_CAST (pool);
}


/**
 * *************************************************************************************************************************************************
 * @brief: 
 * @param xpool: 
 * @return:
 * @note:
 * *************************************************************************************************************************************************
*/
gboolean
gst_x_image_sink_check_xshm_calls (GstXImageSink * ximagesink,
    GstXContext * xcontext)
{
  XImage *ximage;
  XShmSegmentInfo SHMInfo;
  size_t size;
  int (*handler) (Display *, XErrorEvent *);
  gboolean result = FALSE;
  gboolean did_attach = FALSE;

  g_return_val_if_fail (xcontext != NULL, FALSE);

  /* Sync to ensure any older errors are already processed */
  XSync (xcontext->disp, FALSE);

  /* Set defaults so we don't free these later unnecessarily */
  SHMInfo.shmaddr = ((void *) -1);
  SHMInfo.shmid = -1;

  /* Setting an error handler to catch failure */
  error_caught = FALSE;
  handler = XSetErrorHandler (gst_ximagesink_handle_xerror);

  /* Trying to create a 1x1 ximage */
  GST_DEBUG ("XShmCreateImage of 1x1");

  ximage = XShmCreateImage (xcontext->disp, xcontext->visual,
      xcontext->depth, ZPixmap, NULL, &SHMInfo, 1, 1);

  /* Might cause an error, sync to ensure it is noticed */
  XSync (xcontext->disp, FALSE);
  if (!ximage || error_caught) {
    GST_WARNING ("could not XShmCreateImage a 1x1 image");
    goto beach;
  }
  size = ximage->height * ximage->bytes_per_line;

  SHMInfo.shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
  if (SHMInfo.shmid == -1) {
    GST_WARNING ("could not get shared memory of %" G_GSIZE_FORMAT " bytes",
        size);
    goto beach;
  }

  SHMInfo.shmaddr = shmat (SHMInfo.shmid, NULL, 0);
  if (SHMInfo.shmaddr == ((void *) -1)) {
    GST_WARNING ("Failed to shmat: %s", g_strerror (errno));
    /* Clean up the shared memory segment */
    shmctl (SHMInfo.shmid, IPC_RMID, NULL);
    goto beach;
  }

  ximage->data = SHMInfo.shmaddr;
  SHMInfo.readOnly = FALSE;

  if (XShmAttach (xcontext->disp, &SHMInfo) == 0) {
    GST_WARNING ("Failed to XShmAttach");
    /* Clean up the shared memory segment */
    shmctl (SHMInfo.shmid, IPC_RMID, NULL);
    goto beach;
  }

  /* Sync to ensure we see any errors we caused */
  XSync (xcontext->disp, FALSE);

  /* Delete the shared memory segment as soon as everyone is attached.
   * This way, it will be deleted as soon as we detach later, and not
   * leaked if we crash. */
  shmctl (SHMInfo.shmid, IPC_RMID, NULL);

  if (!error_caught) {
    GST_DEBUG ("XServer ShmAttached to 0x%x, id 0x%lx", SHMInfo.shmid,
        SHMInfo.shmseg);

    did_attach = TRUE;
    /* store whether we succeeded in result */
    result = TRUE;
  } else {
    GST_WARNING ("MIT-SHM extension check failed at XShmAttach. "
        "Not using shared memory.");
  }

beach:
  /* Sync to ensure we swallow any errors we caused and reset error_caught */
  XSync (xcontext->disp, FALSE);

  error_caught = FALSE;
  XSetErrorHandler (handler);

  if (did_attach) {
    GST_DEBUG ("XServer ShmDetaching from 0x%x id 0x%lx",
        SHMInfo.shmid, SHMInfo.shmseg);
    XShmDetach (xcontext->disp, &SHMInfo);
    XSync (xcontext->disp, FALSE);
  }
  if (SHMInfo.shmaddr != ((void *) -1))
    shmdt (SHMInfo.shmaddr);
  if (ximage)
    XDestroyImage (ximage);
  return result;
}
#endif /* HAVE_XSHM */
