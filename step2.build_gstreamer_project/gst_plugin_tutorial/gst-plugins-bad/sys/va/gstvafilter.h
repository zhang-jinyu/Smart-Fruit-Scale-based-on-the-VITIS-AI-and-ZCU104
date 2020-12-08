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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "gstvadisplay.h"

#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_VA_FILTER (gst_va_filter_get_type())
G_DECLARE_FINAL_TYPE (GstVaFilter, gst_va_filter, GST, VA_FILTER, GstObject)

enum {
  GST_VA_FILTER_PROP_DENOISE = 1,
  GST_VA_FILTER_PROP_SHARPEN,
  GST_VA_FILTER_PROP_SKINTONE,
  GST_VA_FILTER_PROP_VIDEO_DIR,
  GST_VA_FILTER_PROP_HUE,
  GST_VA_FILTER_PROP_SATURATION,
  GST_VA_FILTER_PROP_BRIGHTNESS,
  GST_VA_FILTER_PROP_CONTRAST,
  GST_VA_FILTER_PROP_AUTO_SATURATION,
  GST_VA_FILTER_PROP_AUTO_BRIGHTNESS,
  GST_VA_FILTER_PROP_AUTO_CONTRAST,
  GST_VA_FILTER_PROP_LAST
};

GstVaFilter *         gst_va_filter_new                   (GstVaDisplay * display);
gboolean              gst_va_filter_open                  (GstVaFilter * self);
gboolean              gst_va_filter_close                 (GstVaFilter * self);
gboolean              gst_va_filter_is_open               (GstVaFilter * self);
gboolean              gst_va_filter_install_properties    (GstVaFilter * self,
                                                           GObjectClass * klass);
gboolean              gst_va_filter_set_orientation       (GstVaFilter * self,
                                                           GstVideoOrientationMethod orientation);
GstVideoOrientationMethod gst_va_filter_get_orientation   (GstVaFilter * self);
const gpointer        gst_va_filter_get_filter_caps       (GstVaFilter * self,
                                                           VAProcFilterType type,
                                                           guint * num_caps);
guint32               gst_va_filter_get_mem_types         (GstVaFilter * self);
GArray *              gst_va_filter_get_surface_formats   (GstVaFilter * self);
GstCaps *             gst_va_filter_get_caps              (GstVaFilter * self);
gboolean              gst_va_filter_add_filter_buffer     (GstVaFilter * self,
                                                           gpointer data,
                                                           gsize size,
                                                           guint num);
gboolean              gst_va_filter_convert_surface       (GstVaFilter * self,
                                                           VASurfaceID in_surface,
                                                           GstVideoInfo * in_info,
                                                           VASurfaceID out_surface,
                                                           GstVideoInfo * out_info);

G_END_DECLS
