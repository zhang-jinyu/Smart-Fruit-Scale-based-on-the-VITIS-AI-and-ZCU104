/* GStreamer
 * Copyright (C) 2020 Seungha Yang <seungha@centricular.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstnvvp9dec.h"
#include "gstcudautils.h"
#include "gstnvdecoder.h"

#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_nv_vp9_dec_debug);
#define GST_CAT_DEFAULT gst_nv_vp9_dec_debug

/* reference list 8 + 2 margin */
#define NUM_OUTPUT_VIEW 10

struct _GstNvVp9Dec
{
  GstVp9Decoder parent;

  GstVideoCodecState *output_state;

  GstCudaContext *context;
  CUstream cuda_stream;
  GstNvDecoder *decoder;
  CUVIDPICPARAMS params;

  guint width, height;
  GstVP9Profile profile;

  GstVideoFormat out_format;

  /* For OpenGL interop. */
  GstObject *gl_display;
  GstObject *gl_context;
  GstObject *other_gl_context;

  GstNvDecoderOutputType output_type;
};

struct _GstNvVp9DecClass
{
  GstVp9DecoderClass parent_class;
  guint cuda_device_id;
};

#define gst_nv_vp9_dec_parent_class parent_class

/**
 * GstNvVp9Dec:
 *
 * Since: 1.20
 */
G_DEFINE_TYPE (GstNvVp9Dec, gst_nv_vp9_dec, GST_TYPE_VP9_DECODER);

static void gst_nv_vp9_dec_set_context (GstElement * element,
    GstContext * context);
static gboolean gst_nv_vp9_dec_open (GstVideoDecoder * decoder);
static gboolean gst_nv_vp9_dec_close (GstVideoDecoder * decoder);
static gboolean gst_nv_vp9_dec_negotiate (GstVideoDecoder * decoder);
static gboolean gst_nv_vp9_dec_decide_allocation (GstVideoDecoder *
    decoder, GstQuery * query);
static gboolean gst_nv_vp9_dec_src_query (GstVideoDecoder * decoder,
    GstQuery * query);

/* GstVp9Decoder */
static gboolean gst_nv_vp9_dec_new_sequence (GstVp9Decoder * decoder,
    const GstVp9Parser * parser, const GstVp9FrameHdr * frame_hdr);
static gboolean gst_nv_vp9_dec_new_picture (GstVp9Decoder * decoder,
    GstVideoCodecFrame * frame, GstVp9Picture * picture);
static GstVp9Picture *gst_nv_vp9_dec_duplicate_picture (GstVp9Decoder *
    decoder, GstVp9Picture * picture);
static gboolean gst_nv_vp9_dec_decode_picture (GstVp9Decoder * decoder,
    GstVp9Picture * picture, GstVp9Dpb * dpb);
static GstFlowReturn gst_nv_vp9_dec_output_picture (GstVp9Decoder *
    decoder, GstVideoCodecFrame * frame, GstVp9Picture * picture);

static void
gst_nv_vp9_dec_class_init (GstNvVp9DecClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoDecoderClass *decoder_class = GST_VIDEO_DECODER_CLASS (klass);
  GstVp9DecoderClass *vp9decoder_class = GST_VP9_DECODER_CLASS (klass);

  element_class->set_context = GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_set_context);

  decoder_class->open = GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_open);
  decoder_class->close = GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_close);
  decoder_class->negotiate = GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_negotiate);
  decoder_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_decide_allocation);
  decoder_class->src_query = GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_src_query);

  vp9decoder_class->new_sequence =
      GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_new_sequence);
  vp9decoder_class->new_picture =
      GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_new_picture);
  vp9decoder_class->duplicate_picture =
      GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_duplicate_picture);
  vp9decoder_class->decode_picture =
      GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_decode_picture);
  vp9decoder_class->output_picture =
      GST_DEBUG_FUNCPTR (gst_nv_vp9_dec_output_picture);

  GST_DEBUG_CATEGORY_INIT (gst_nv_vp9_dec_debug,
      "nvvp9dec", 0, "NVIDIA VP9 Decoder");

  gst_type_mark_as_plugin_api (GST_TYPE_NV_VP9_DEC, 0);
}

static void
gst_nv_vp9_dec_init (GstNvVp9Dec * self)
{
}

static void
gst_nv_vp9_dec_set_context (GstElement * element, GstContext * context)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (element);
  GstNvVp9DecClass *klass = GST_NV_VP9_DEC_GET_CLASS (self);

  GST_DEBUG_OBJECT (self, "set context %s",
      gst_context_get_context_type (context));

  gst_nv_decoder_set_context (element, context, klass->cuda_device_id,
      &self->context, &self->gl_display, &self->other_gl_context);

  GST_ELEMENT_CLASS (parent_class)->set_context (element, context);
}

static gboolean
gst_nv_vp9_dec_open (GstVideoDecoder * decoder)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);
  GstNvVp9DecClass *klass = GST_NV_VP9_DEC_GET_CLASS (self);

  if (!gst_nv_decoder_ensure_element_data (GST_ELEMENT (self),
          klass->cuda_device_id, &self->context, &self->cuda_stream,
          &self->gl_display, &self->other_gl_context)) {
    GST_ERROR_OBJECT (self, "Required element data is unavailable");
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_nv_vp9_dec_close (GstVideoDecoder * decoder)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);

  g_clear_pointer (&self->output_state, gst_video_codec_state_unref);
  gst_clear_object (&self->decoder);

  if (self->context && self->cuda_stream) {
    if (gst_cuda_context_push (self->context)) {
      gst_cuda_result (CuStreamDestroy (self->cuda_stream));
      gst_cuda_context_pop (NULL);
    }
  }

  gst_clear_object (&self->gl_context);
  gst_clear_object (&self->other_gl_context);
  gst_clear_object (&self->gl_display);
  gst_clear_object (&self->context);
  self->cuda_stream = NULL;

  return TRUE;
}

static gboolean
gst_nv_vp9_dec_negotiate (GstVideoDecoder * decoder)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);
  GstVp9Decoder *vp9dec = GST_VP9_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "negotiate");

  gst_nv_decoder_negotiate (decoder, vp9dec->input_state, self->out_format,
      self->width, self->height, self->gl_display, self->other_gl_context,
      &self->gl_context, &self->output_state, &self->output_type);

  /* TODO: add support D3D11 memory */

  return GST_VIDEO_DECODER_CLASS (parent_class)->negotiate (decoder);
}

static gboolean
gst_nv_vp9_dec_decide_allocation (GstVideoDecoder * decoder, GstQuery * query)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);

  gst_nv_decoder_decide_allocation (self->decoder, decoder, query,
      self->gl_context, self->output_type);

  return GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation
      (decoder, query);
}

static gboolean
gst_nv_vp9_dec_src_query (GstVideoDecoder * decoder, GstQuery * query)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONTEXT:
      if (gst_nv_decoder_handle_context_query (GST_ELEMENT (self), query,
              self->context, self->gl_display, self->gl_context,
              self->other_gl_context)) {
        return TRUE;
      }
      break;
    default:
      break;
  }

  return GST_VIDEO_DECODER_CLASS (parent_class)->src_query (decoder, query);
}

static gboolean
gst_nv_vp9_dec_new_sequence (GstVp9Decoder * decoder,
    const GstVp9Parser * parser, const GstVp9FrameHdr * frame_hdr)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);
  gboolean modified = FALSE;

  GST_LOG_OBJECT (self, "new sequence");

  if (self->width != frame_hdr->width || self->height != frame_hdr->height) {
    if (self->decoder) {
      GST_INFO_OBJECT (self, "resolution changed %dx%d -> %dx%d",
          self->width, self->height, frame_hdr->width, frame_hdr->height);
    }

    self->width = frame_hdr->width;
    self->height = frame_hdr->height;
    modified = TRUE;
  }

  if (self->profile != frame_hdr->profile) {
    if (self->decoder) {
      GST_INFO_OBJECT (self, "profile changed %d -> %d", self->profile,
          frame_hdr->profile);
    }

    self->profile = frame_hdr->profile;
    modified = TRUE;
  }

  if (modified || !self->decoder) {
    GstVideoInfo info;

    gst_clear_object (&self->decoder);

    self->out_format = GST_VIDEO_FORMAT_UNKNOWN;

    if (self->profile == GST_VP9_PROFILE_0) {
      self->out_format = GST_VIDEO_FORMAT_NV12;
    } else if (self->profile == GST_VP9_PROFILE_2) {
      if (parser->bit_depth == 10)
        self->out_format = GST_VIDEO_FORMAT_P010_10LE;
      else
        self->out_format = GST_VIDEO_FORMAT_P016_LE;
    }

    if (self->out_format == GST_VIDEO_FORMAT_UNKNOWN) {
      GST_ERROR_OBJECT (self, "Could not support profile %d", self->profile);
      return FALSE;
    }

    gst_video_info_set_format (&info,
        self->out_format, self->width, self->height);

    self->decoder = gst_nv_decoder_new (self->context, cudaVideoCodec_VP9,
        &info, NUM_OUTPUT_VIEW);

    if (!self->decoder) {
      GST_ERROR_OBJECT (self, "Failed to create decoder");
      return FALSE;
    }

    if (!gst_video_decoder_negotiate (GST_VIDEO_DECODER (self))) {
      GST_ERROR_OBJECT (self, "Failed to negotiate with downstream");
      return FALSE;
    }

    memset (&self->params, 0, sizeof (CUVIDPICPARAMS));

    self->params.CodecSpecific.vp9.colorSpace = parser->color_space;
  }

  return TRUE;
}

static gboolean
gst_nv_vp9_dec_new_picture (GstVp9Decoder * decoder,
    GstVideoCodecFrame * frame, GstVp9Picture * picture)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);
  GstNvDecoderFrame *nv_frame;

  nv_frame = gst_nv_decoder_new_frame (self->decoder);
  if (!nv_frame) {
    GST_ERROR_OBJECT (self, "No available decoder frame");
    return FALSE;
  }

  GST_LOG_OBJECT (self,
      "New decoder frame %p (index %d)", nv_frame, nv_frame->index);

  gst_vp9_picture_set_user_data (picture,
      nv_frame, (GDestroyNotify) gst_nv_decoder_frame_unref);

  return TRUE;
}

static GstNvDecoderFrame *
gst_nv_vp9_dec_get_decoder_frame_from_picture (GstNvVp9Dec * self,
    GstVp9Picture * picture)
{
  GstNvDecoderFrame *frame;

  frame = (GstNvDecoderFrame *) gst_vp9_picture_get_user_data (picture);

  if (!frame)
    GST_DEBUG_OBJECT (self, "current picture does not have decoder frame");

  return frame;
}

static GstVp9Picture *
gst_nv_vp9_dec_duplicate_picture (GstVp9Decoder * decoder,
    GstVp9Picture * picture)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);
  GstNvDecoderFrame *frame;
  GstVp9Picture *new_picture;

  frame = gst_nv_vp9_dec_get_decoder_frame_from_picture (self, picture);

  if (!frame) {
    GST_ERROR_OBJECT (self, "Parent picture does not have decoder frame");
    return NULL;
  }

  new_picture = gst_vp9_picture_new ();
  new_picture->frame_hdr = picture->frame_hdr;

  gst_vp9_picture_set_user_data (new_picture,
      gst_nv_decoder_frame_ref (frame),
      (GDestroyNotify) gst_nv_decoder_frame_unref);

  return new_picture;
}

static gboolean
gst_nv_vp9_dec_decode_picture (GstVp9Decoder * decoder,
    GstVp9Picture * picture, GstVp9Dpb * dpb)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);
  const GstVp9FrameHdr *frame_hdr = &picture->frame_hdr;
  const GstVp9LoopFilter *loopfilter = &frame_hdr->loopfilter;
  const GstVp9SegmentationInfo *seg = &frame_hdr->segmentation;
  const GstVp9QuantIndices *quant_indices = &frame_hdr->quant_indices;
  CUVIDPICPARAMS *params = &self->params;
  CUVIDVP9PICPARAMS *vp9_params = &params->CodecSpecific.vp9;
  GstNvDecoderFrame *frame;
  GstNvDecoderFrame *other_frame;
  guint offset = 0;
  guint8 ref_frame_map[GST_VP9_REF_FRAMES];
  gint i;

  G_STATIC_ASSERT (G_N_ELEMENTS (vp9_params->mbRefLfDelta) ==
      GST_VP9_MAX_REF_LF_DELTAS);
  G_STATIC_ASSERT (G_N_ELEMENTS (vp9_params->mbModeLfDelta) ==
      GST_VP9_MAX_MODE_LF_DELTAS);
  G_STATIC_ASSERT (G_N_ELEMENTS (vp9_params->mb_segment_tree_probs) ==
      GST_VP9_SEG_TREE_PROBS);
  G_STATIC_ASSERT (G_N_ELEMENTS (vp9_params->refFrameSignBias) ==
      GST_VP9_REFS_PER_FRAME + 1);
  G_STATIC_ASSERT (G_N_ELEMENTS (vp9_params->activeRefIdx) ==
      GST_VP9_REFS_PER_FRAME);
  G_STATIC_ASSERT (G_N_ELEMENTS (vp9_params->segmentFeatureEnable) ==
      GST_VP9_MAX_SEGMENTS);
  G_STATIC_ASSERT (G_N_ELEMENTS (vp9_params->segmentFeatureData) ==
      GST_VP9_MAX_SEGMENTS);

  GST_LOG_OBJECT (self, "Decode picture, size %" G_GSIZE_FORMAT, picture->size);

  frame = gst_nv_vp9_dec_get_decoder_frame_from_picture (self, picture);
  if (!frame) {
    GST_ERROR_OBJECT (self, "Decoder frame is unavailable");
    return FALSE;
  }

  params->nBitstreamDataLen = picture->size;
  params->pBitstreamData = picture->data;
  params->nNumSlices = 1;
  params->pSliceDataOffsets = &offset;

  params->PicWidthInMbs = GST_ROUND_UP_16 (frame_hdr->width) >> 4;
  params->FrameHeightInMbs = GST_ROUND_UP_16 (frame_hdr->height) >> 4;
  params->CurrPicIdx = frame->index;

  vp9_params->width = frame_hdr->width;
  vp9_params->height = frame_hdr->height;

  for (i = 0; i < GST_VP9_REF_FRAMES; i++) {
    if (dpb->pic_list[i]) {
      other_frame = gst_nv_vp9_dec_get_decoder_frame_from_picture (self,
          dpb->pic_list[i]);
      if (!other_frame) {
        GST_ERROR_OBJECT (self, "Couldn't get decoder frame from picture");
        return FALSE;
      }

      ref_frame_map[i] = other_frame->index;
    } else {
      ref_frame_map[i] = 0xff;
    }
  }

  vp9_params->LastRefIdx = ref_frame_map[frame_hdr->ref_frame_indices[0]];
  vp9_params->GoldenRefIdx = ref_frame_map[frame_hdr->ref_frame_indices[1]];
  vp9_params->AltRefIdx = ref_frame_map[frame_hdr->ref_frame_indices[2]];

  vp9_params->profile = frame_hdr->profile;
  vp9_params->frameContextIdx = frame_hdr->frame_context_idx;
  vp9_params->frameType = frame_hdr->frame_type;
  vp9_params->showFrame = frame_hdr->show_frame;
  vp9_params->errorResilient = frame_hdr->error_resilient_mode;
  vp9_params->frameParallelDecoding = frame_hdr->frame_parallel_decoding_mode;
  vp9_params->subSamplingX = picture->subsampling_x;
  vp9_params->subSamplingY = picture->subsampling_y;
  vp9_params->intraOnly = frame_hdr->intra_only;
  vp9_params->allow_high_precision_mv = frame_hdr->allow_high_precision_mv;
  vp9_params->refreshEntropyProbs = frame_hdr->refresh_frame_context;
  vp9_params->bitDepthMinus8Luma = picture->bit_depth - 8;
  vp9_params->bitDepthMinus8Chroma = picture->bit_depth - 8;

  vp9_params->loopFilterLevel = loopfilter->filter_level;
  vp9_params->loopFilterSharpness = loopfilter->sharpness_level;
  vp9_params->modeRefLfEnabled = loopfilter->mode_ref_delta_enabled;

  vp9_params->log2_tile_columns = frame_hdr->log2_tile_columns;
  vp9_params->log2_tile_rows = frame_hdr->log2_tile_rows;

  vp9_params->segmentEnabled = seg->enabled;
  vp9_params->segmentMapUpdate = seg->update_map;
  vp9_params->segmentMapTemporalUpdate = seg->temporal_update;
  vp9_params->segmentFeatureMode = seg->abs_delta;

  vp9_params->qpYAc = quant_indices->y_ac_qi;
  vp9_params->qpYDc = quant_indices->y_dc_delta;
  vp9_params->qpChDc = quant_indices->uv_dc_delta;
  vp9_params->qpChAc = quant_indices->uv_ac_delta;

  vp9_params->resetFrameContext = frame_hdr->reset_frame_context;
  vp9_params->mcomp_filter_type = frame_hdr->mcomp_filter_type;
  vp9_params->frameTagSize = frame_hdr->frame_header_length_in_bytes;
  vp9_params->offsetToDctParts = frame_hdr->first_partition_size;

  for (i = 0; i < GST_VP9_MAX_REF_LF_DELTAS; i++)
    vp9_params->mbRefLfDelta[i] = loopfilter->ref_deltas[i];

  for (i = 0; i < GST_VP9_MAX_MODE_LF_DELTAS; i++)
    vp9_params->mbModeLfDelta[i] = loopfilter->mode_deltas[i];

  for (i = 0; i < GST_VP9_SEG_TREE_PROBS; i++)
    vp9_params->mb_segment_tree_probs[i] = seg->tree_probs[i];

  vp9_params->refFrameSignBias[0] = 0;
  for (i = 0; i < GST_VP9_REFS_PER_FRAME; i++) {
    vp9_params->refFrameSignBias[i + 1] = frame_hdr->ref_frame_sign_bias[i];
    vp9_params->activeRefIdx[i] = frame_hdr->ref_frame_indices[i];
  }

  for (i = 0; i < GST_VP9_MAX_SEGMENTS; i++) {
    vp9_params->segmentFeatureEnable[i][0] =
        seg->data[i].alternate_quantizer_enabled;
    vp9_params->segmentFeatureEnable[i][1] =
        seg->data[i].alternate_loop_filter_enabled;
    vp9_params->segmentFeatureEnable[i][2] =
        seg->data[i].reference_frame_enabled;
    vp9_params->segmentFeatureEnable[i][3] = seg->data[i].reference_skip;

    vp9_params->segmentFeatureData[i][0] = seg->data[i].alternate_quantizer;
    vp9_params->segmentFeatureData[i][1] = seg->data[i].alternate_loop_filter;
    vp9_params->segmentFeatureData[i][2] = seg->data[i].reference_frame;
    vp9_params->segmentFeatureData[i][3] = 0;
  }

  return gst_nv_decoder_decode_picture (self->decoder, &self->params);
}

static GstFlowReturn
gst_nv_vp9_dec_output_picture (GstVp9Decoder * decoder,
    GstVideoCodecFrame * frame, GstVp9Picture * picture)
{
  GstNvVp9Dec *self = GST_NV_VP9_DEC (decoder);
  GstVideoDecoder *vdec = GST_VIDEO_DECODER (decoder);
  GstNvDecoderFrame *decoder_frame;
  gboolean ret G_GNUC_UNUSED = FALSE;

  GST_LOG_OBJECT (self, "Outputting picture %p", picture);

  decoder_frame = (GstNvDecoderFrame *) gst_vp9_picture_get_user_data (picture);
  if (!decoder_frame) {
    GST_ERROR_OBJECT (self, "No decoder frame in picture %p", picture);
    goto error;
  }

  frame->output_buffer = gst_video_decoder_allocate_output_buffer (vdec);
  if (!frame->output_buffer) {
    GST_ERROR_OBJECT (self, "Couldn't allocate output buffer");
    goto error;
  }

  if (self->output_type == GST_NV_DECODER_OUTPUT_TYPE_GL) {
    ret = gst_nv_decoder_finish_frame (self->decoder,
        GST_NV_DECODER_OUTPUT_TYPE_GL, self->gl_context,
        decoder_frame, frame->output_buffer);

    /* FIXME: This is the case where OpenGL context of downstream glbufferpool
     * belongs to non-nvidia (or different device).
     * There should be enhancement to ensure nvdec has compatible OpenGL context
     */
    if (!ret) {
      GST_WARNING_OBJECT (self,
          "Couldn't copy frame to GL memory, fallback to system memory");
      self->output_type = GST_NV_DECODER_OUTPUT_TYPE_SYSTEM;
    }
  }

  if (!ret) {
    if (!gst_nv_decoder_finish_frame (self->decoder,
            self->output_type, NULL, decoder_frame, frame->output_buffer)) {
      GST_ERROR_OBJECT (self, "Failed to finish frame");
      goto error;
    }
  }

  gst_vp9_picture_unref (picture);

  return gst_video_decoder_finish_frame (vdec, frame);

error:
  gst_video_decoder_drop_frame (vdec, frame);
  gst_vp9_picture_unref (picture);

  return GST_FLOW_ERROR;
}

typedef struct
{
  GstCaps *sink_caps;
  GstCaps *src_caps;
  guint cuda_device_id;
  gboolean is_default;
} GstNvVp9DecClassData;

static void
gst_nv_vp9_dec_subclass_init (gpointer klass, GstNvVp9DecClassData * cdata)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstNvVp9DecClass *nvdec_class = (GstNvVp9DecClass *) (klass);
  gchar *long_name;

  if (cdata->is_default) {
    long_name = g_strdup_printf ("NVDEC VP9 Stateless Decoder");
  } else {
    long_name = g_strdup_printf ("NVDEC VP9 Stateless Decoder with device %d",
        cdata->cuda_device_id);
  }

  gst_element_class_set_metadata (element_class, long_name,
      "Codec/Decoder/Video/Hardware",
      "NVIDIA VP9 video decoder", "Seungha Yang <seungha@centricular.com>");
  g_free (long_name);

  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          cdata->sink_caps));
  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          cdata->src_caps));

  nvdec_class->cuda_device_id = cdata->cuda_device_id;

  gst_caps_unref (cdata->sink_caps);
  gst_caps_unref (cdata->src_caps);
  g_free (cdata);
}

void
gst_nv_vp9_dec_register (GstPlugin * plugin, guint device_id, guint rank,
    GstCaps * sink_caps, GstCaps * src_caps, gboolean is_primary)
{
  GTypeQuery type_query;
  GTypeInfo type_info = { 0, };
  GType subtype;
  gchar *type_name;
  gchar *feature_name;
  GstNvVp9DecClassData *cdata;
  gboolean is_default = TRUE;

  /**
   * element-nvvp9sldec:
   *
   * Since: 1.20
   */

  cdata = g_new0 (GstNvVp9DecClassData, 1);
  cdata->sink_caps = gst_caps_copy (sink_caps);
  gst_caps_set_simple (cdata->sink_caps,
      "alignment", G_TYPE_STRING, "frame", NULL);
  GST_MINI_OBJECT_FLAG_SET (cdata->sink_caps,
      GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
  cdata->src_caps = gst_caps_ref (src_caps);
  cdata->cuda_device_id = device_id;

  g_type_query (GST_TYPE_NV_VP9_DEC, &type_query);
  memset (&type_info, 0, sizeof (type_info));
  type_info.class_size = type_query.class_size;
  type_info.instance_size = type_query.instance_size;
  type_info.class_init = (GClassInitFunc) gst_nv_vp9_dec_subclass_init;
  type_info.class_data = cdata;

  if (is_primary) {
    type_name = g_strdup ("GstNvVP9StatelessPrimaryDec");
    feature_name = g_strdup ("nvvp9dec");
  } else {
    type_name = g_strdup ("GstNvVP9StatelessDec");
    feature_name = g_strdup ("nvvp9sldec");
  }

  if (g_type_from_name (type_name) != 0) {
    g_free (type_name);
    g_free (feature_name);
    if (is_primary) {
      type_name =
          g_strdup_printf ("GstNvVP9StatelessPrimaryDevice%dDec", device_id);
      feature_name = g_strdup_printf ("nvvp9device%ddec", device_id);
    } else {
      type_name = g_strdup_printf ("GstNvVP9StatelessDevice%dDec", device_id);
      feature_name = g_strdup_printf ("nvvp9sldevice%ddec", device_id);
    }

    is_default = FALSE;
  }

  cdata->is_default = is_default;
  subtype = g_type_register_static (GST_TYPE_NV_VP9_DEC,
      type_name, &type_info, 0);

  /* make lower rank than default device */
  if (rank > 0 && !is_default)
    rank--;

  if (!gst_element_register (plugin, feature_name, rank, subtype))
    GST_WARNING ("Failed to register plugin '%s'", type_name);

  g_free (type_name);
  g_free (feature_name);
}
