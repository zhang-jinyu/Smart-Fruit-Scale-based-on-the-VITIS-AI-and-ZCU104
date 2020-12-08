/* GStreamer
 * Copyright (C) 2020 FIXME <fixme@example.com>
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

#ifndef _GST_VAITFSSD_H_
#define _GST_VAITFSSD_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_VAITFSSD   (gst_vaitfssd_get_type())
#define GST_VAITFSSD(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VAITFSSD,GstVaitfssd))
#define GST_VAITFSSD_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VAITFSSD,GstVaitfssdClass))
#define GST_IS_VAITFSSD(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VAITFSSD))
#define GST_IS_VAITFSSD_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VAITFSSD))

typedef struct _GstVaitfssd GstVaitfssd;
typedef struct _GstVaitfssdClass GstVaitfssdClass;

struct _GstVaitfssd
{
  GstVideoFilter base_vaitfssd;

};

struct _GstVaitfssdClass
{
  GstVideoFilterClass base_vaitfssd_class;
};

GType gst_vaitfssd_get_type (void);

G_END_DECLS

#endif
