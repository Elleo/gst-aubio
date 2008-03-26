/*
 
    Copyright (C) 2008 Paul Brossier <piem@piem.org>

    This file is part of gst-aubio.

    gst-aubio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    gst-aubio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with gst-aubio.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <gst/gst.h>
#include "gstaubiotempo.h"

#define GST_CAT_DEFAULT gst_aubiotempo_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_aubiotempo_debug, "aubiotempo",
      0, "Aubiotempo plugin");

  return gst_element_register (plugin, "aubiotempo",
      GST_RANK_NONE, GST_TYPE_AUBIOTEMPO);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "aubio",
    "Aubio plugin",
    plugin_init, VERSION, "GPL", "GStreamer-aubio", "http://aubio.org/")

