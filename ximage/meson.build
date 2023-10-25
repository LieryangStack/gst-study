project('gstreamer-ximagesink', 'c',
  version : '1.22.6',
  meson_version : '>= 0.62.0',
  default_options : ['buildtype=debugoptimized',
                     # Needed due to https://github.com/mesonbuild/meson/issues/1889,
                     # but this can cause problems in the future. Remove it
                     # when it's no longer necessary.
                     'cpp_std=c++14'])

gst_plugins_base_args = ['-DHAVE_CONFIG_H']
if get_option('default_library') == 'static'
  gst_plugins_base_args += ['-DGST_STATIC_COMPILATION']
endif

cc = meson.get_compiler('c')

no_warn_args = []
# XKeycodeToKeysym is deprecated, but we use it when Xkb is unavailable
if cc.has_argument ('-Wno-deprecated-declarations')
  no_warn_args += '-Wno-deprecated-declarations'
endif

plugins = []
configinc = include_directories('.')
plugins_install_dir = join_paths(get_option('libdir'), 'gstreamer-1.0')

x11_dep = dependency('x11', required : get_option('x11'))

gstvideo_dep = dependency('gstreamer-video-1.0', version : '1.22.6',
    fallback : ['gst-plugins-base', 'video_dep'])
gst_dep = dependency('gstreamer-1.0', version : '1.22.6',
  fallback : ['gstreamer', 'gst_dep'])
gst_base_dep = dependency('gstreamer-base-1.0', version : '1.22.6',
  fallback : ['gstreamer', 'gst_base_dep'])

if x11_dep.found()
  # XShm is only used by ximage and xvimage
  # FIXME: Need to check for XShmAttach inside libXext
  xshm_dep = dependency('xext', required : get_option('xshm'))
  xi_dep = dependency('xi', required : get_option('xi'))
  # core_conf.set('HAVE_XSHM', xshm_dep.found())
  # core_conf.set('HAVE_XI2', xi_dep.found())

  # subdir('ximage')

  gstximage = library('gstximagesink',
    'ximagesink.c', 'ximage.c', 'ximagepool.c',
    c_args : gst_plugins_base_args + no_warn_args,
    include_directories: [configinc],
    dependencies : [gstvideo_dep, gst_base_dep, gst_dep, x11_dep, xshm_dep, xi_dep],
    install : true,
    install_dir : plugins_install_dir,
  )
  plugins += [gstximage]
endif
