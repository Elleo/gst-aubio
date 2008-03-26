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

#ifndef __GST_AUBIO_PITCH_H__
#define __GST_AUBIO_PITCH_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/gstaudiofilter.h>

#include <aubio/aubio.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_AUBIO_PITCH \
  (gst_aubio_pitch_get_type())
#define GST_AUBIO_PITCH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_AUBIO_PITCH,GstAubioPitch))
#define GST_IS_AUBIO_PITCH(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_AUBIO_PITCH))
#define GST_AUBIO_PITCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_AUBIO_PITCH,GstAubioPitchClass))
#define GST_IS_AUBIO_PITCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_AUBIO_PITCH))
#define GST_AUBIO_PITCH_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_AUBIO_PITCH, GstAubioPitchClass))

typedef struct _GstAubioPitch      GstAubioPitch;
typedef struct _GstAubioPitchClass GstAubioPitchClass;

struct _GstAubioPitch
{
  GstAudioFilter element;

  GstPad *sinkpad, *srcpad;

  gboolean silent;

  aubio_pitchdetection_t * t;
  fvec_t * ibuf;

  uint buf_size;
  uint hop_size;
  uint channels;
  uint samplerate;
  uint pos;

  aubio_pitchdetection_type type_pitch;
  aubio_pitchdetection_mode mode_pitch;

};

struct _GstAubioPitchClass 
{
  GstAudioFilterClass parent_class;
};

GType gst_aubio_pitch_get_type (void);

G_END_DECLS

#endif /* __GST_AUBIO_PITCH_H__ */
