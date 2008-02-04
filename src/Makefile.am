# plugindir is set in configure

##############################################################################
# change libgstplugin.la to something more suitable, e.g. libmysomething.la  #
##############################################################################
plugin_LTLIBRARIES = libgstaubio.la

##############################################################################
# for the next set of variables, rename the prefix if you renamed the .la,   #
#  e.g. libgstplugin_la_SOURCES => libmysomething_la_SOURCES                 #
#       libgstplugin_la_CFLAGS  => libmysomething_la_CFLAGS                  #
#       libgstplugin_la_LIBADD  => libmysomething_la_LIBADD                  #
#       libgstplugin_la_LDFLAGS => libmysomething_la_LDFLAGS                 #
##############################################################################

# sources used to compile this plug-in
libgstaubio_la_SOURCES = gstaubiotempo.c

# flags used to compile the aubio gst plugin
# add other _CFLAGS and _LIBS as needed
libgstaubio_la_CFLAGS = $(GST_CFLAGS)
libgstaubio_la_LIBADD = $(GST_LIBS)
libgstaubio_la_LDFLAGS = $(GST_AUBIO_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstaubiotempo.h
