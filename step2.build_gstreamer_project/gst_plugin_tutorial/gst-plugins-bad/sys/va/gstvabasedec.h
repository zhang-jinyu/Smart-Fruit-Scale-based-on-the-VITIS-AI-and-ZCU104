
/* GStreamer
 * Copyright (C) 2020 Igalia, S.L.
 *     Author: Víctor Jáquez <vjaquez@igalia.com>
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
 * License along with this library; if not, write to the0
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include <gst/codecs/gsth264decoder.h>
#include <gst/codecs/gsth265decoder.h>
#include <gst/codecs/gstvp8decoder.h>
#include <gst/codecs/gstvp9decoder.h>

#include "gstvadevice.h"
#include "gstvadecoder.h"
#include "gstvaprofile.h"

G_BEGIN_DECLS

#define GST_VA_BASE_DEC(obj) ((GstVaBaseDec *)(obj))
#define GST_VA_BASE_DEC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_FROM_INSTANCE (obj), GstVaBaseDecClass))
#define GST_VA_BASE_DEC_CLASS(klass) ((GstVaBaseDecClass *)(klass))

typedef struct _GstVaBaseDec GstVaBaseDec;
typedef struct _GstVaBaseDecClass GstVaBaseDecClass;

struct _GstVaBaseDec
{
  /* <private> */
  union
  {
    GstH264Decoder h264;
    GstH265Decoder h265;
    GstVp8Decoder vp8;
    GstVp9Decoder vp9;
  } parent;

  GstDebugCategory *debug_category;

  GstVaDisplay *display;
  GstVaDecoder *decoder;

  VAProfile profile;
  guint rt_format;
  gint width;
  gint height;

  guint min_buffers;

  GstVideoCodecState *output_state;
  GstBufferPool *other_pool;

  gboolean need_valign;
  GstVideoAlignment valign;

  gboolean copy_frames;
};

struct _GstVaBaseDecClass
{
  /* <private> */
  union
  {
    GstH264DecoderClass h264;
    GstH265DecoderClass h265;
    GstVp8DecoderClass vp8;
    GstVp9DecoderClass vp9;
  } parent_class;

  GstVaCodecs codec;
  gchar *render_device_path;
};

struct CData
{
  gchar *render_device_path;
  gchar *description;
  GstCaps *sink_caps;
  GstCaps *src_caps;
};

void                  gst_va_base_dec_init                (GstVaBaseDec * base,
                                                           GstDebugCategory * cat);
void                  gst_va_base_dec_class_init          (GstVaBaseDecClass * klass,
                                                           GstVaCodecs codec,
                                                           const gchar * render_device_path,
                                                           GstCaps * sink_caps,
                                                           GstCaps * src_caps,
                                                           GstCaps * doc_src_caps,
                                                           GstCaps * doc_sink_caps);

gboolean              gst_va_base_dec_close               (GstVideoDecoder * decoder);
void                  gst_va_base_dec_get_preferred_format_and_caps_features (GstVaBaseDec * base,
                                                           GstVideoFormat * format,
                                                           GstCapsFeatures ** capsfeatures);
gboolean              gst_va_base_dec_copy_output_buffer  (GstVaBaseDec * base,
                                                           GstVideoCodecFrame * codec_frame);

G_END_DECLS
