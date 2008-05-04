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
 * SECTION:element-aubiopitch
 *
 * <refsect2>
 * Detects beats along an audio stream 
 * <title>Example launch line</title>
 * <para>
 * <programlisting>
 * gst-launch -v -m audiotestsrc ! aubiopitch ! fakesink silent=TRUE
 * </programlisting>
 * </para>
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>

#include "gstaubiopitch.h"

GST_DEBUG_CATEGORY_STATIC(aubiopitch_debug);
#define GST_CAT_DEFAULT aubiopitch_debug

static const GstElementDetails element_details = 
GST_ELEMENT_DETAILS ("Aubio Pitch Analysis",
  "Filter/Analyzer/Audio",
  "Extract pitch using aubio",
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
    " rate=(int)44100,"                                               \
    " channels=(int)1"

GST_BOILERPLATE (GstAubioPitch, gst_aubio_pitch, GstAudioFilter,
    GST_TYPE_AUDIO_FILTER);

static void gst_aubio_pitch_finalize (GObject * obj);
static void gst_aubio_pitch_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_aubio_pitch_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_aubio_pitch_transform_ip (GstBaseTransform * trans,
        GstBuffer * buf);

/* GObject vmethod implementations */
static void
gst_aubio_pitch_base_init (gpointer gclass)
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
gst_aubio_pitch_class_init (GstAubioPitchClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass *trans_class = GST_BASE_TRANSFORM_CLASS (klass);
  //GstAudioFilterClass *filter_class = GST_AUDIO_FILTER_CLASS (klass);

  //trans_class->stop = GST_DEBUG_FUNCPTR (gst_aubio_pitch_stop);
  //trans_class->event = GST_DEBUG_FUNCPTR (gst_aubio_pitch_event);
  trans_class->transform_ip = GST_DEBUG_FUNCPTR (gst_aubio_pitch_transform_ip);
  trans_class->passthrough_on_same_caps = TRUE;

  gobject_class->finalize = gst_aubio_pitch_finalize;
  gobject_class->set_property = gst_aubio_pitch_set_property;
  gobject_class->get_property = gst_aubio_pitch_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output",
          TRUE, G_PARAM_READWRITE));

  GST_DEBUG_CATEGORY_INIT (aubiopitch_debug, "aubiopitch", 0,
          "Aubio pitch extraction");

}

static void
gst_aubio_pitch_init (GstAubioPitch * filter,
    GstAubioPitchClass * gclass)
{

  filter->silent = TRUE;

  filter->type_pitch = aubio_pitch_yinfft;
  filter->mode_pitch = aubio_pitchm_freq;

  filter->buf_size = 2048;
  filter->hop_size = 256;
  filter->samplerate = 44100;
  filter->channels = 1;

  filter->ibuf = new_fvec(filter->hop_size, filter->channels);
  filter->t = new_aubio_pitchdetection(filter->buf_size, filter->hop_size,
      filter->channels, filter->samplerate, filter->type_pitch, filter->mode_pitch);
  aubio_pitchdetection_set_yinthresh(filter->t, 0.7);
}

static void
gst_aubio_pitch_finalize (GObject * obj)
{
  GstAubioPitch * aubio_pitch = GST_AUBIO_PITCH (obj);

  if (aubio_pitch->t) {
    del_aubio_pitchdetection(aubio_pitch->t);
  }
  if (aubio_pitch->ibuf) {
    del_fvec(aubio_pitch->ibuf);
  }

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_aubio_pitch_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAubioPitch *filter = GST_AUBIO_PITCH (object);

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
gst_aubio_pitch_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAubioPitch *filter = GST_AUBIO_PITCH (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_aubio_pitch_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  uint j;
  GstAubioPitch *filter = GST_AUBIO_PITCH (trans);
  GstAudioFilter *audiofilter = GST_AUDIO_FILTER(trans);

  gint nsamples = GST_BUFFER_SIZE (buf) / (4 * audiofilter->format.channels);

  /* block loop */
  for (j = 0; j < nsamples; j++) {
    /* copy input to ibuf */
    fvec_write_sample(filter->ibuf, ((smpl_t *) GST_BUFFER_DATA(buf))[j], 0,
        filter->pos);

    if (filter->pos == filter->hop_size - 1) {
      smpl_t pitch = aubio_pitchdetection(filter->t, filter->ibuf);
      GstClockTime now = GST_BUFFER_TIMESTAMP (buf);
      // correction of inside buffer time
      now += GST_FRAMES_TO_CLOCK_TIME(j, audiofilter->format.rate);

      if (filter->silent == FALSE) {
        g_print ("%" GST_TIME_FORMAT "\tpitch: %.3f\n",
                GST_TIME_ARGS(now), pitch);


      }

      GST_LOG_OBJECT (filter, "pitch %" GST_TIME_FORMAT ", freq %3.2f",
              GST_TIME_ARGS(now), pitch);

      filter->pos = -1; /* so it will be zero next j loop */
    }
    filter->pos++;
  }

  return GST_FLOW_OK;
}
