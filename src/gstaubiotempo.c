/*
 * GStreamer
 * Copyright 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-aubiotempo
 *
 * <refsect2>
 * Detects beats along an audio stream 
 * <title>Example launch line</title>
 * <para>
 * <programlisting>
 * gst-launch -v -m audiotestsrc ! aubiotempo ! fakesink silent=TRUE
 * </programlisting>
 * </para>
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <aubio/aubio.h>

#include <gst/gst.h>
#include <gst/audio/audio.h>

#include "gstaubiotempo.h"

#define GST_CAT_DEFAULT gst_aubiotempo_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static const GstElementDetails element_details = 
GST_ELEMENT_DETAILS ("Aubio Tempo Analysis",
  "Filter/Analyzer/Audio",
  "Extract tempo period and beat locations using aubio",
  "Paul Brossier <piem@aubio.org>");

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT
};

#define ALLOWED_CAPS \
    "audio/x-raw-float,"                                              \
    " width=(int)32,"                                                 \
    " endianness=(int)BYTE_ORDER,"                                    \
    " rate=(int)44100,"                                             \
    " channels=(int)[1,MAX]"

/*
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );
*/

GST_BOILERPLATE (GstAubioTempo, gst_aubiotempo, GstAudioFilter,
    GST_TYPE_AUDIO_FILTER);

static void gst_aubiotempo_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_aubiotempo_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

//static gboolean gst_aubiotempo_set_caps (GstPad * pad, GstCaps * caps);
static GstFlowReturn gst_aubiotempo_transform_ip (GstBaseTransform * trans, GstBuffer * buf);

/* GObject vmethod implementations */
static void
gst_aubiotempo_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  GstCaps *caps;

  caps = gst_caps_from_string (ALLOWED_CAPS);

  gst_audio_filter_class_add_pad_templates (GST_AUDIO_FILTER_CLASS (gclass),
        caps);

  gst_caps_unref (caps);

  /*
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
  */

  gst_element_class_set_details (element_class, &element_details);

}

/* initialize the plugin's class */
static void
gst_aubiotempo_class_init (GstAubioTempoClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  //GstAudioFilterClass *filter_class = GST_AUDIO_FILTER_CLASS (klass);

  //trans_class->stop = GST_DEBUG_FUNCPTR (gst_aubiotempo_stop);
  //trans_class->event = GST_DEBUG_FUNCPTR (gst_aubiotempo_event);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_aubiotempo_transform_ip);
  trans_class->passthrough_on_same_caps = TRUE;

  gobject_class->set_property = gst_aubiotempo_set_property;
  gobject_class->get_property = gst_aubiotempo_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set functions
 * initialize structure
 */
static void
gst_aubiotempo_init (GstAubioTempo * filter,
    GstAubioTempoClass * gclass)
{

  filter->silent = TRUE;

  filter->type_onset = aubio_onset_kl;

  filter->buf_size = 1024;
  filter->hop_size = 512;
  filter->channels = 1;

  filter->ibuf = new_fvec(filter->hop_size, filter->channels);
  filter->out = new_fvec(2,filter->channels);
  filter->t = new_aubio_tempo(filter->type_onset,
          filter->buf_size, filter->hop_size, filter->channels);
}

static void
gst_aubiotempo_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAubioTempo *filter = GST_AUBIOTEMPO (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_aubiotempo_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAubioTempo *filter = GST_AUBIOTEMPO (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
/*
static gboolean
gst_aubiotempo_set_caps (GstPad * pad, GstCaps * caps)
{
  GstAubioTempo *filter;
  GstPad *otherpad;

  filter = GST_AUBIOTEMPO (gst_pad_get_parent (pad));
  otherpad = (pad == filter->srcpad) ? filter->sinkpad : filter->srcpad;

  return gst_pad_set_caps (pad, caps);
}
*/

/* chain function
 * this function does the actual processing
 */

static GstFlowReturn
gst_aubiotempo_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  uint j;
  GstAubioTempo *filter = GST_AUBIOTEMPO (trans);
  GstAudioFilter *audiofilter = GST_AUDIO_FILTER(trans);

  gint nsamples = GST_BUFFER_SIZE (buf) / (4 * audiofilter->format.channels);

  /* block loop */
  for (j = 0; j < nsamples; j++) {
    /* copy input to ibuf */
    fvec_write_sample(filter->ibuf, (smpl_t)(GST_BUFFER_DATA(buf)[j]), 0, filter->pos);

    if (filter->pos == filter->hop_size - 1) {
      aubio_tempo(filter->t, filter->ibuf, filter->out);

      if (filter->out->data[0][0]==1) {
        gint64 now = GST_BUFFER_TIMESTAMP (buf);
        // correction of inside buffer time
        //now += GST_FRAMES_TO_CLOCK_TIME(j, audiofilter->format.rate);
        if (filter->silent == FALSE) {
            g_print ("beat: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(now));
        }
      }

      filter->pos = -1; /* so it will be zero next j loop */
    }
    filter->pos++;
  }

  return GST_FLOW_OK;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and pad templates
 * register the features
 */
static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_aubiotempo_debug, "aubiotempo",
      0, "Aubiotempo plugin");

  return gst_element_register (plugin, "aubiotempo",
      GST_RANK_NONE, GST_TYPE_AUBIOTEMPO);
}

/* this is the structure that gstreamer looks for to register plugins
 *
 * exchange the strings 'plugin' and 'Template plugin' with you plugin name and
 * description
 */
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "aubiotempo",
    "Aubiotempo plugin",
    plugin_init, VERSION, "GPL", "GStreamer-aubio", "http://aubio.org/")
