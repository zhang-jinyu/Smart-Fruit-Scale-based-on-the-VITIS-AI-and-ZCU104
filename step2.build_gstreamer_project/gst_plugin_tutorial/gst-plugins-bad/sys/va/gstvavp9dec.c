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

/**
 * SECTION:element-vavp9dec
 * @title: vavp9dec
 * @short_description: A VA-API based VP9 video decoder
 *
 * vavp9dec decodes VP9 bitstreams to VA surfaces using the
 * installed and chosen [VA-API](https://01.org/linuxmedia/vaapi)
 * driver.
 *
 * The decoding surfaces can be mapped onto main memory as video
 * frames.
 *
 * ## Example launch line
 * ```
 * gst-launch-1.0 filesrc location=sample.webm ! parsebin ! vavp9dec ! autovideosink
 * ```
 *
 * Since: 1.20
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstvavp9dec.h"

#include "gstvabasedec.h"

GST_DEBUG_CATEGORY_STATIC (gst_va_vp9dec_debug);
#ifndef GST_DISABLE_GST_DEBUG
#define GST_CAT_DEFAULT gst_va_vp9dec_debug
#else
#define GST_CAT_DEFAULT NULL
#endif

#define GST_VA_VP9_DEC(obj)           ((GstVaVp9Dec *) obj)
#define GST_VA_VP9_DEC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_FROM_INSTANCE (obj), GstVaVp9DecClass))
#define GST_VA_VP9_DEC_CLASS(klass)   ((GstVaVp9DecClass *) klass)

typedef struct _GstVaVp9Dec GstVaVp9Dec;
typedef struct _GstVaVp9DecClass GstVaVp9DecClass;

struct _GstVaVp9DecClass
{
  GstVaBaseDecClass parent_class;
};

struct _GstVaVp9Dec
{
  GstVaBaseDec parent;

  gboolean need_negotiation;
};

#define parent_class gst_va_base_dec_parent_class
extern gpointer gst_va_base_dec_parent_class;

/* *INDENT-OFF* */
static const gchar *src_caps_str = GST_VIDEO_CAPS_MAKE_WITH_FEATURES ("memory:VAMemory",
            "{ NV12, P010_10LE }") " ;" GST_VIDEO_CAPS_MAKE ("{ NV12, P010_10LE }");
/* *INDENT-ON* */

static const gchar *sink_caps_str = "video/x-vp9";

static guint
_get_rtformat (GstVaVp9Dec * self, GstVP9Profile profile,
    GstVp9BitDepth bit_depth, gint subsampling_x, gint subsampling_y)
{
  switch (profile) {
    case GST_VP9_PROFILE_0:
      return VA_RT_FORMAT_YUV420;
    case GST_VP9_PROFILE_1:
      if (subsampling_x == 1 && subsampling_y == 0)
        return VA_RT_FORMAT_YUV422;
      else if (subsampling_x == 0 && subsampling_y == 0)
        return VA_RT_FORMAT_YUV444;
      break;
    case GST_VP9_PROFILE_2:
      if (bit_depth == GST_VP9_BIT_DEPTH_10)
        return VA_RT_FORMAT_YUV420_10;
      else if (bit_depth == GST_VP9_BIT_DEPTH_12)
        return VA_RT_FORMAT_YUV420_12;
      break;
    case GST_VP9_PROFILE_3:
      if (subsampling_x == 1 && subsampling_y == 0) {
        if (bit_depth == GST_VP9_BIT_DEPTH_10)
          return VA_RT_FORMAT_YUV422_10;
        else if (bit_depth == GST_VP9_BIT_DEPTH_12)
          return VA_RT_FORMAT_YUV422_12;
      } else if (subsampling_x == 0 && subsampling_y == 0) {
        if (bit_depth == GST_VP9_BIT_DEPTH_10)
          return VA_RT_FORMAT_YUV444_10;
        else if (bit_depth == GST_VP9_BIT_DEPTH_12)
          return VA_RT_FORMAT_YUV444_12;
      }
      break;
    default:
      break;
  }

  GST_ERROR_OBJECT (self, "Unsupported chroma format");
  return 0;
}

static VAProfile
_get_profile (GstVaVp9Dec * self, GstVP9Profile profile)
{
  switch (profile) {
    case GST_VP9_PROFILE_0:
      return VAProfileVP9Profile0;
    case GST_VP9_PROFILE_1:
      return VAProfileVP9Profile1;
    case GST_VP9_PROFILE_2:
      return VAProfileVP9Profile2;
    case GST_VP9_PROFILE_3:
      return VAProfileVP9Profile3;
    default:
      break;
  }

  GST_ERROR_OBJECT (self, "Unsupported profile");
  return VAProfileNone;
}

static gboolean
gst_va_vp9_new_sequence (GstVp9Decoder * decoder, const GstVp9Parser * parser,
    const GstVp9FrameHdr * frame_hdr)
{
  GstVaBaseDec *base = GST_VA_BASE_DEC (decoder);
  GstVaVp9Dec *self = GST_VA_VP9_DEC (decoder);
  VAProfile profile;
  gboolean negotiation_needed = FALSE;
  guint rt_format;

  profile = _get_profile (self, frame_hdr->profile);
  if (profile == VAProfileNone)
    return FALSE;

  if (!gst_va_decoder_has_profile (base->decoder, profile)) {
    GST_ERROR_OBJECT (self, "Profile %s is not supported",
        gst_va_profile_name (profile));
    return FALSE;
  }

  rt_format = _get_rtformat (self, frame_hdr->profile, parser->bit_depth,
      parser->subsampling_x, parser->subsampling_y);
  if (rt_format == 0)
    return FALSE;

  if (gst_va_decoder_format_changed (base->decoder, profile,
          rt_format, frame_hdr->width, frame_hdr->height)) {
    base->profile = profile;
    base->width = frame_hdr->width;
    base->height = frame_hdr->height;
    base->rt_format = rt_format;
    negotiation_needed = TRUE;
  }

  base->min_buffers = GST_VP9_REF_FRAMES;

  if (negotiation_needed) {
    self->need_negotiation = TRUE;
    if (!gst_video_decoder_negotiate (GST_VIDEO_DECODER (self))) {
      GST_ERROR_OBJECT (self, "Failed to negotiate with downstream");
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
gst_va_vp9_dec_new_picture (GstVp9Decoder * decoder,
    GstVideoCodecFrame * frame, GstVp9Picture * picture)
{
  GstFlowReturn ret;
  GstVaVp9Dec *self = GST_VA_VP9_DEC (decoder);
  GstVaDecodePicture *pic;
  GstVideoDecoder *vdec = GST_VIDEO_DECODER (decoder);

  ret = gst_video_decoder_allocate_output_frame (vdec, frame);
  if (ret != GST_FLOW_OK)
    goto error;

  pic = gst_va_decode_picture_new (frame->output_buffer);

  gst_vp9_picture_set_user_data (picture, pic,
      (GDestroyNotify) gst_va_decode_picture_free);

  GST_LOG_OBJECT (self, "New va decode picture %p - %#x", pic,
      gst_va_decode_picture_get_surface (pic));

  return TRUE;

error:
  {
    GST_WARNING_OBJECT (self, "Failed to allocated output buffer, return %s",
        gst_flow_get_name (ret));
    return FALSE;
  }
}

static inline gboolean
_fill_param (GstVp9Decoder * decoder, GstVp9Picture * picture, GstVp9Dpb * dpb)
{
  GstVaBaseDec *base = GST_VA_BASE_DEC (decoder);
  GstVaDecodePicture *va_pic;
  const GstVp9FrameHdr *frame_hdr = &picture->frame_hdr;
  const GstVp9LoopFilter *loopfilter = &frame_hdr->loopfilter;
  const GstVp9SegmentationInfo *seg = &frame_hdr->segmentation;
  VADecPictureParameterBufferVP9 pic_param;
  guint i;

  /* *INDENT-OFF* */
  pic_param = (VADecPictureParameterBufferVP9) {
    .frame_width = base->width,
    .frame_height = base->height,

    .pic_fields.bits = {
      .subsampling_x = picture->subsampling_x,
      .subsampling_y = picture->subsampling_x,
      .frame_type = frame_hdr->frame_type,
      .show_frame = frame_hdr->show_frame,
      .error_resilient_mode = frame_hdr->error_resilient_mode,
      .intra_only = frame_hdr->intra_only,
      .allow_high_precision_mv = (frame_hdr->frame_type == GST_VP9_KEY_FRAME) ?
          0 : frame_hdr->allow_high_precision_mv,
      .mcomp_filter_type = frame_hdr->mcomp_filter_type,
      .frame_parallel_decoding_mode = frame_hdr->frame_parallel_decoding_mode,
      .reset_frame_context = frame_hdr->reset_frame_context,
      .refresh_frame_context = frame_hdr->refresh_frame_context,
      .frame_context_idx = frame_hdr->frame_context_idx,

      .segmentation_enabled = seg->enabled,
      .segmentation_temporal_update = seg->temporal_update,
      .segmentation_update_map = seg->update_map,

      .last_ref_frame =
          frame_hdr->ref_frame_indices[GST_VP9_REF_FRAME_LAST - 1],
      .last_ref_frame_sign_bias =
          frame_hdr->ref_frame_sign_bias[GST_VP9_REF_FRAME_LAST - 1],
      .golden_ref_frame =
          frame_hdr->ref_frame_indices[GST_VP9_REF_FRAME_GOLDEN - 1],
      .golden_ref_frame_sign_bias =
          frame_hdr->ref_frame_sign_bias[GST_VP9_REF_FRAME_GOLDEN - 1],
      .alt_ref_frame =
          frame_hdr->ref_frame_indices[GST_VP9_REF_FRAME_ALTREF - 1],
      .alt_ref_frame_sign_bias =
          frame_hdr->ref_frame_sign_bias[GST_VP9_REF_FRAME_ALTREF - 1],

      .lossless_flag = frame_hdr->lossless_flag,
    },

    .filter_level = loopfilter->filter_level,
    .sharpness_level = loopfilter->sharpness_level,
    .log2_tile_rows = frame_hdr->log2_tile_rows,
    .log2_tile_columns = frame_hdr->log2_tile_columns,

    .frame_header_length_in_bytes = frame_hdr->frame_header_length_in_bytes,
    .first_partition_size = frame_hdr->first_partition_size,

    .profile = frame_hdr->profile,
    .bit_depth = picture->bit_depth
  };
  /* *INDENT-ON* */

  memcpy (pic_param.mb_segment_tree_probs, seg->tree_probs,
      sizeof (seg->tree_probs));

  if (seg->temporal_update) {
    memcpy (pic_param.segment_pred_probs, seg->pred_probs,
        sizeof (seg->pred_probs));
  } else {
    memset (pic_param.segment_pred_probs, 255,
        sizeof (pic_param.segment_pred_probs));
  }

  for (i = 0; i < GST_VP9_REF_FRAMES; i++) {
    if (dpb->pic_list[i]) {
      GstVaDecodePicture *va_pic =
          gst_vp9_picture_get_user_data (dpb->pic_list[i]);

      pic_param.reference_frames[i] =
          gst_va_decode_picture_get_surface (va_pic);
    } else {
      pic_param.reference_frames[i] = VA_INVALID_ID;
    }
  }

  va_pic = gst_vp9_picture_get_user_data (picture);

  return gst_va_decoder_add_param_buffer (base->decoder, va_pic,
      VAPictureParameterBufferType, &pic_param, sizeof (pic_param));
}

static inline gboolean
_fill_slice (GstVp9Decoder * decoder, GstVp9Picture * picture)
{
  GstVaBaseDec *base = GST_VA_BASE_DEC (decoder);
  GstVaDecodePicture *va_pic;
  const GstVp9Segmentation *seg = picture->segmentation;
  VASliceParameterBufferVP9 slice_param;
  guint i;

  /* *INDENT-OFF* */
  slice_param = (VASliceParameterBufferVP9) {
    .slice_data_size = picture->size,
    .slice_data_offset = 0,
    .slice_data_flag = VA_SLICE_DATA_FLAG_ALL,
  };
  /* *INDENT-ON* */

  for (i = 0; i < GST_VP9_MAX_SEGMENTS; i++) {
    /* *INDENT-OFF* */
    slice_param.seg_param[i] = (VASegmentParameterVP9) {
        .segment_flags.fields = {
            .segment_reference_enabled = seg[i].reference_frame_enabled,
            .segment_reference = seg[i].reference_frame,
            .segment_reference_skipped = seg[i].reference_skip,
         },

        .luma_dc_quant_scale = seg[i].luma_dc_quant_scale,
        .luma_ac_quant_scale = seg[i].luma_ac_quant_scale,
        .chroma_dc_quant_scale = seg[i].chroma_dc_quant_scale,
        .chroma_ac_quant_scale = seg[i].chroma_ac_quant_scale,
    };
    /* *INDENT-ON* */

    memcpy (slice_param.seg_param[i].filter_level, seg[i].filter_level,
        sizeof (slice_param.seg_param[i].filter_level));
  }

  va_pic = gst_vp9_picture_get_user_data (picture);

  return gst_va_decoder_add_slice_buffer (base->decoder, va_pic, &slice_param,
      sizeof (slice_param), (gpointer) picture->data, picture->size);

}

static gboolean
gst_va_vp9_decode_picture (GstVp9Decoder * decoder, GstVp9Picture * picture,
    GstVp9Dpb * dpb)
{
  GstVaBaseDec *base = GST_VA_BASE_DEC (decoder);

  if (!_fill_param (decoder, picture, dpb))
    goto fail;

  if (!_fill_slice (decoder, picture))
    goto fail;

  return TRUE;

fail:
  {
    GstVaDecodePicture *va_pic;

    va_pic = gst_vp9_picture_get_user_data (picture);
    gst_va_decoder_destroy_buffers (base->decoder, va_pic);
    return FALSE;
  }
}

static gboolean
gst_va_vp9_dec_end_picture (GstVp9Decoder * decoder, GstVp9Picture * picture)
{
  GstVaBaseDec *base = GST_VA_BASE_DEC (decoder);
  GstVaDecodePicture *va_pic;

  GST_LOG_OBJECT (base, "end picture %p", picture);

  va_pic = gst_vp9_picture_get_user_data (picture);

  return gst_va_decoder_decode (base->decoder, va_pic);
}

static GstFlowReturn
gst_va_vp9_dec_output_picture (GstVp9Decoder * decoder,
    GstVideoCodecFrame * frame, GstVp9Picture * picture)
{
  GstVaBaseDec *base = GST_VA_BASE_DEC (decoder);
  GstVaVp9Dec *self = GST_VA_VP9_DEC (decoder);

  GST_LOG_OBJECT (self, "Outputting picture %p", picture);

  if (base->copy_frames)
    gst_va_base_dec_copy_output_buffer (base, frame);

  gst_vp9_picture_unref (picture);

  return gst_video_decoder_finish_frame (GST_VIDEO_DECODER (self), frame);
}

static GstVp9Picture *
gst_va_vp9_dec_duplicate_picture (GstVp9Decoder * decoder,
    GstVp9Picture * picture)
{
  GstVaDecodePicture *va_pic, *va_dup;
  GstVp9Picture *new_picture;

  va_pic = gst_vp9_picture_get_user_data (picture);
  va_dup = gst_va_decode_picture_dup (va_pic);

  new_picture = gst_vp9_picture_new ();
  new_picture->frame_hdr = picture->frame_hdr;

  gst_vp9_picture_set_user_data (picture, va_dup,
      (GDestroyNotify) gst_va_decode_picture_free);

  return new_picture;
}

static gboolean
gst_va_vp9_dec_negotiate (GstVideoDecoder * decoder)
{
  GstCapsFeatures *capsfeatures = NULL;
  GstVaBaseDec *base = GST_VA_BASE_DEC (decoder);
  GstVaVp9Dec *self = GST_VA_VP9_DEC (decoder);
  GstVideoFormat format = GST_VIDEO_FORMAT_UNKNOWN;
  GstVp9Decoder *vp9dec = GST_VP9_DECODER (decoder);

  /* Ignore downstream renegotiation request. */
  if (!self->need_negotiation)
    return TRUE;

  self->need_negotiation = FALSE;

  if (gst_va_decoder_is_open (base->decoder)
      && !gst_va_decoder_close (base->decoder))
    return FALSE;

  if (!gst_va_decoder_open (base->decoder, base->profile, base->rt_format))
    return FALSE;

  if (!gst_va_decoder_set_format (base->decoder, base->width, base->height,
          NULL))
    return FALSE;

  if (base->output_state)
    gst_video_codec_state_unref (base->output_state);

  gst_va_base_dec_get_preferred_format_and_caps_features (base, &format,
      &capsfeatures);

  base->output_state =
      gst_video_decoder_set_output_state (decoder, format,
      base->width, base->height, vp9dec->input_state);

  base->output_state->caps = gst_video_info_to_caps (&base->output_state->info);
  if (capsfeatures)
    gst_caps_set_features_simple (base->output_state->caps, capsfeatures);

  GST_INFO_OBJECT (self, "Negotiated caps %" GST_PTR_FORMAT,
      base->output_state->caps);

  return GST_VIDEO_DECODER_CLASS (parent_class)->negotiate (decoder);
}

static void
gst_va_vp9_dec_dispose (GObject * object)
{
  gst_va_base_dec_close (GST_VIDEO_DECODER (object));
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_va_vp9_dec_class_init (gpointer g_class, gpointer class_data)
{
  GstCaps *src_doc_caps, *sink_doc_caps;
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  GstVideoDecoderClass *decoder_class = GST_VIDEO_DECODER_CLASS (g_class);
  GstVp9DecoderClass *vp9_class = GST_VP9_DECODER_CLASS (g_class);
  struct CData *cdata = class_data;
  gchar *long_name;

  if (cdata->description) {
    long_name = g_strdup_printf ("VA-API VP9 Decoder in %s",
        cdata->description);
  } else {
    long_name = g_strdup ("VA-API VP9 Decoder");
  }

  gst_element_class_set_metadata (element_class, long_name,
      "Codec/Decoder/Video/Hardware", "VA-API based VP9 video decoder",
      "Víctor Jáquez <vjaquez@igalia.com>");

  sink_doc_caps = gst_caps_from_string (sink_caps_str);
  src_doc_caps = gst_caps_from_string (src_caps_str);

  gst_va_base_dec_class_init (GST_VA_BASE_DEC_CLASS (g_class), VP9,
      cdata->render_device_path, cdata->sink_caps, cdata->src_caps,
      src_doc_caps, sink_doc_caps);

  gobject_class->dispose = gst_va_vp9_dec_dispose;

  decoder_class->negotiate = GST_DEBUG_FUNCPTR (gst_va_vp9_dec_negotiate);

  vp9_class->new_sequence = GST_DEBUG_FUNCPTR (gst_va_vp9_new_sequence);
  vp9_class->new_picture = GST_DEBUG_FUNCPTR (gst_va_vp9_dec_new_picture);
  vp9_class->decode_picture = GST_DEBUG_FUNCPTR (gst_va_vp9_decode_picture);
  vp9_class->end_picture = GST_DEBUG_FUNCPTR (gst_va_vp9_dec_end_picture);
  vp9_class->output_picture = GST_DEBUG_FUNCPTR (gst_va_vp9_dec_output_picture);
  vp9_class->duplicate_picture =
      GST_DEBUG_FUNCPTR (gst_va_vp9_dec_duplicate_picture);

  g_free (long_name);
  g_free (cdata->description);
  g_free (cdata->render_device_path);
  gst_caps_unref (cdata->src_caps);
  gst_caps_unref (cdata->sink_caps);
  g_free (cdata);
}

static void
gst_va_vp9_dec_init (GTypeInstance * instance, gpointer g_class)
{
  gst_va_base_dec_init (GST_VA_BASE_DEC (instance), GST_CAT_DEFAULT);
}

/* This element doesn't parse supreframes. Let's delegate it to the
 * parser. */
static GstCaps *
_complete_sink_caps (GstCaps * sinkcaps)
{
  gst_caps_set_simple (sinkcaps, "alignment", G_TYPE_STRING, "frame", NULL);
  return gst_caps_ref (sinkcaps);
}

static gpointer
_register_debug_category (gpointer data)
{
  GST_DEBUG_CATEGORY_INIT (gst_va_vp9dec_debug, "vavp9dec", 0,
      "VA VP9 decoder");

  return NULL;
}

gboolean
gst_va_vp9_dec_register (GstPlugin * plugin, GstVaDevice * device,
    GstCaps * sink_caps, GstCaps * src_caps, guint rank)
{
  static GOnce debug_once = G_ONCE_INIT;
  GType type;
  GTypeInfo type_info = {
    .class_size = sizeof (GstVaVp9DecClass),
    .class_init = gst_va_vp9_dec_class_init,
    .instance_size = sizeof (GstVaVp9Dec),
    .instance_init = gst_va_vp9_dec_init,
  };
  struct CData *cdata;
  gboolean ret;
  gchar *type_name, *feature_name;

  g_return_val_if_fail (GST_IS_PLUGIN (plugin), FALSE);
  g_return_val_if_fail (GST_IS_VA_DEVICE (device), FALSE);
  g_return_val_if_fail (GST_IS_CAPS (sink_caps), FALSE);
  g_return_val_if_fail (GST_IS_CAPS (src_caps), FALSE);

  cdata = g_new (struct CData, 1);
  cdata->description = NULL;
  cdata->render_device_path = g_strdup (device->render_device_path);
  cdata->sink_caps = _complete_sink_caps (sink_caps);
  cdata->src_caps = gst_caps_ref (src_caps);

  /* class data will be leaked if the element never gets instantiated */
  GST_MINI_OBJECT_FLAG_SET (sink_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
  GST_MINI_OBJECT_FLAG_SET (src_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);

  type_info.class_data = cdata;

  type_name = g_strdup ("GstVaVp9Dec");
  feature_name = g_strdup ("vavp9dec");

  /* The first decoder to be registered should use a constant name,
   * like vavp9dec, for any additional decoders, we create unique
   * names, using inserting the render device name. */
  if (g_type_from_name (type_name)) {
    gchar *basename = g_path_get_basename (device->render_device_path);
    g_free (type_name);
    g_free (feature_name);
    type_name = g_strdup_printf ("GstVa%sVp9Dec", basename);
    feature_name = g_strdup_printf ("va%svp9dec", basename);
    cdata->description = basename;

    /* lower rank for non-first device */
    if (rank > 0)
      rank--;
  }

  g_once (&debug_once, _register_debug_category, NULL);

  type = g_type_register_static (GST_TYPE_VP9_DECODER,
      type_name, &type_info, 0);

  ret = gst_element_register (plugin, feature_name, rank, type);

  g_free (type_name);
  g_free (feature_name);

  return ret;
}
