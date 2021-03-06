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

/**
 * SECTION:element-aubiotempo
 *
 * <refsect2>
 * Detects beats along an audio stream 
 * <title>Example launch line</title>
 * <para>
 * <programlisting>
 * gst-launch -v -m audiotestsrc ! aubiotempo ! fakesink
 * gst-launch filesrc location=audiofile ! decodebin ! audioconvert ! \
 *      aubiotempo silent=FALSE ! audioconvert ! autoaudiosink
 * </programlisting>
 * </para>
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>

#include "gstaubiotempo.h"

GST_DEBUG_CATEGORY_STATIC(aubiotempo_debug);
#define GST_CAT_DEFAULT aubiotempo_debug

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
  PROP_SILENT,
  PROP_MESSAGE,
};

#define ALLOWED_CAPS \
    "audio/x-raw-float,"                                              \
    " width=(int)32,"                                                 \
    " endianness=(int)BYTE_ORDER,"                                    \
    " rate=(int)44100,"                                               \
    " channels=(int)1"

GST_BOILERPLATE (GstAubioTempo, gst_aubio_tempo, GstAudioFilter,
    GST_TYPE_AUDIO_FILTER);

static void gst_aubio_tempo_finalize (GObject * obj);
static void gst_aubio_tempo_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_aubio_tempo_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_aubio_tempo_transform_ip (GstBaseTransform * trans,
        GstBuffer * buf);

/* GObject vmethod implementations */
static void
gst_aubio_tempo_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  GstCaps *caps;

  caps = gst_caps_from_string (ALLOWED_CAPS);

  gst_audio_filter_class_add_pad_templates (GST_AUDIO_FILTER_CLASS (gclass),
        caps);

  gst_caps_unref (caps);

  gst_element_class_set_details (element_class, &element_details);

}

/* initialize the plugin's class */
static void
gst_aubio_tempo_class_init (GstAubioTempoClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  //GstAudioFilterClass *filter_class = GST_AUDIO_FILTER_CLASS (klass);

  //trans_class->stop = GST_DEBUG_FUNCPTR (gst_aubio_tempo_stop);
  //trans_class->event = GST_DEBUG_FUNCPTR (gst_aubio_tempo_event);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_aubio_tempo_transform_ip);
  trans_class->passthrough_on_same_caps = TRUE;

  gobject_class->finalize = gst_aubio_tempo_finalize;
  gobject_class->set_property = gst_aubio_tempo_set_property;
  gobject_class->get_property = gst_aubio_tempo_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output",
          TRUE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MESSAGE,
      g_param_spec_boolean ("message", "Message", "Emit gstreamer messages",
          TRUE, G_PARAM_READWRITE));

  GST_DEBUG_CATEGORY_INIT (aubiotempo_debug, "aubiotempo", 0,
          "Aubio tempo extraction");

}

static void
gst_aubio_tempo_init (GstAubioTempo * filter,
    GstAubioTempoClass * gclass)
{

  filter->silent = TRUE;
  filter->message = TRUE;

  filter->buf_size = 1024;
  filter->hop_size = 128;
  filter->channels = 1;

  filter->last_beat = -1;
  filter->bpm = 0;

  filter->ibuf = new_fvec(filter->hop_size);
  filter->out = new_fvec(2);
  filter->t = new_aubio_tempo("kl",
          filter->buf_size, filter->hop_size, 44100);
}

static void
gst_aubio_tempo_finalize (GObject * obj)
{
  GstAubioTempo * aubio_tempo = GST_AUBIOTEMPO (obj);

  if (aubio_tempo->t) {
    del_aubio_tempo(aubio_tempo->t);
  }
  if (aubio_tempo->ibuf) {
    del_fvec(aubio_tempo->ibuf);
  }
  if (aubio_tempo->out) {
    del_fvec(aubio_tempo->out);
  }

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_aubio_tempo_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAubioTempo *filter = GST_AUBIOTEMPO (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_MESSAGE:
      filter->message = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_aubio_tempo_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAubioTempo *filter = GST_AUBIOTEMPO (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_MESSAGE:
      g_value_set_boolean (value, filter->message);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstMessage *
gst_aubio_tempo_message_new(GstAubioTempo *a, GstClockTime beat)
{
  GstStructure *s;
  s = gst_structure_new("aubiotempo", 
          "beat", GST_TYPE_CLOCK_TIME, beat  ,
          "bpm" , G_TYPE_DOUBLE      , a->bpm,
          NULL);

  return gst_message_new_element (GST_OBJECT (a), s);
}

static GstFlowReturn
gst_aubio_tempo_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  uint j;
  GstAubioTempo *filter = GST_AUBIOTEMPO(trans);
  GstAudioFilter *audiofilter = GST_AUDIO_FILTER(trans);

  gint nsamples = GST_BUFFER_SIZE (buf) / (4 * audiofilter->format.channels);

  /* block loop */
  for (j = 0; j < nsamples; j++) {
    /* copy input to ibuf */
    fvec_write_sample(filter->ibuf, ((smpl_t *) GST_BUFFER_DATA(buf))[j],
        filter->pos);

    if (filter->pos == filter->hop_size - 1) {
      aubio_tempo_do(filter->t, filter->ibuf, filter->out);

      if (filter->out->data[0]> 0.) {
        gdouble now = GST_BUFFER_OFFSET (buf);
        // correction of inside buffer time
        now += (smpl_t)(j - filter->hop_size + 1);
        // correction of float period
        now += (filter->out->data[0] - 1.)*(smpl_t)filter->hop_size;

        if (filter->last_beat != -1 && now > filter->last_beat) {
          filter->bpm = 60./(GST_FRAMES_TO_CLOCK_TIME(now - filter->last_beat, audiofilter->format.rate))*1.e+9;
        } else {
          filter->bpm = 0.;
        }

        if (filter->silent == FALSE) {
          g_print ("beat: %f ", GST_FRAMES_TO_CLOCK_TIME( now, audiofilter->format.rate)*1.e-9);
          g_print ("| bpm: %f\n", filter->bpm);
        }

        GST_LOG_OBJECT (filter, "beat %" GST_TIME_FORMAT ", bpm %3.2f",
            GST_TIME_ARGS(now), filter->bpm);

        if (filter->message) {
          GstMessage *m = gst_aubio_tempo_message_new (filter, now);
          gst_element_post_message (GST_ELEMENT (filter), m);
        }

        filter->last_beat = now;
      }

      filter->pos = -1; /* so it will be zero next j loop */
    }
    filter->pos++;
  }

  return GST_FLOW_OK;
}
