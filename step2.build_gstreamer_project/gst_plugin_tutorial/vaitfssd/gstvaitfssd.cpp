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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstvaitfssd
 *
 * The vaitfssd element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! vaitfssd ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstvaitfssd.h"

    /* OpenCV header files */
    #include <opencv2/core.hpp>
    #include <opencv2/opencv.hpp>
    #include <opencv2/highgui.hpp>
    #include <opencv2/imgproc.hpp>

    /* Vitis-AI-Library specific header files */
    #include <vitis/ai/tfssd.hpp>
    #include <vitis/ai/nnpp/tfssd.hpp>
using namespace std;
const string classes[80]= {"person","bicycle","car","motobike","aeroplane","bus","train","truck","boat","traffic light",
"fire hydrant","stop sign","parking meter","bench","bird","cat","dog","horse","sheep","cow",
"elephant","bear","zebra","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee",
"skis","snowboard","sports ball","kite","baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle",
"wine glass","cup","fork","knife","spoon","bowl","banana","apple","sandwich","orange",
"broccoli","carrot","hot dog","pizza","donut","cake","chair","sofa","pottedplant","bed",
"diningtable","toilet","tvmonitor","laptop","mouse","remote","keyboard","cell phone","micowave","oven",
"toaster","sink","refrigerator","book","clock","vase","scissors","teddy bear","hair drier","toothbrush"
};
string class_label;

GST_DEBUG_CATEGORY_STATIC (gst_vaitfssd_debug_category);
#define GST_CAT_DEFAULT gst_vaitfssd_debug_category

/* prototypes */


static void gst_vaitfssd_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_vaitfssd_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_vaitfssd_dispose (GObject * object);
static void gst_vaitfssd_finalize (GObject * object);

static gboolean gst_vaitfssd_start (GstBaseTransform * trans);
static gboolean gst_vaitfssd_stop (GstBaseTransform * trans);
static gboolean gst_vaitfssd_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_vaitfssd_transform_frame (GstVideoFilter * filter,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn gst_vaitfssd_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame);

enum
{
  PROP_0
};

/* pad templates */

/* FIXME: add/remove formats you can handle */
#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")

/* FIXME: add/remove formats you can handle */
#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ BGR }")


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstVaitfssd, gst_vaitfssd, GST_TYPE_VIDEO_FILTER,
  GST_DEBUG_CATEGORY_INIT (gst_vaitfssd_debug_category, "vaitfssd", 0,
  "debug category for vaitfssd element"));

static void
gst_vaitfssd_class_init (GstVaitfssdClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
        gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SRC_CAPS ",width = (int) [1, 640], height = (int) [1, 360]")));
    gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
        gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SINK_CAPS ", width = (int) [1, 640], height = (int) [1, 360]")));

    gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
        "ssd detection using the Vitis-AI-Library", 
        "Video Filter", 
        "ssd Detection",
        "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_vaitfssd_set_property;
  gobject_class->get_property = gst_vaitfssd_get_property;
  gobject_class->dispose = gst_vaitfssd_dispose;
  gobject_class->finalize = gst_vaitfssd_finalize;
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_vaitfssd_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_vaitfssd_stop);
  video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_vaitfssd_set_info);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_vaitfssd_transform_frame_ip);

}

static void
gst_vaitfssd_init (GstVaitfssd *vaitfssd)
{
}

void
gst_vaitfssd_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVaitfssd *vaitfssd = GST_VAITFSSD (object);

  GST_DEBUG_OBJECT (vaitfssd, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_vaitfssd_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstVaitfssd *vaitfssd = GST_VAITFSSD (object);

  GST_DEBUG_OBJECT (vaitfssd, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_vaitfssd_dispose (GObject * object)
{
  GstVaitfssd *vaitfssd = GST_VAITFSSD (object);

  GST_DEBUG_OBJECT (vaitfssd, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_vaitfssd_parent_class)->dispose (object);
}

void
gst_vaitfssd_finalize (GObject * object)
{
  GstVaitfssd *vaitfssd = GST_VAITFSSD (object);

  GST_DEBUG_OBJECT (vaitfssd, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_vaitfssd_parent_class)->finalize (object);
}

static gboolean
gst_vaitfssd_start (GstBaseTransform * trans)
{
  GstVaitfssd *vaitfssd = GST_VAITFSSD (trans);

  GST_DEBUG_OBJECT (vaitfssd, "start");

  return TRUE;
}

static gboolean
gst_vaitfssd_stop (GstBaseTransform * trans)
{
  GstVaitfssd *vaitfssd = GST_VAITFSSD (trans);

  GST_DEBUG_OBJECT (vaitfssd, "stop");

  return TRUE;
}

static gboolean
gst_vaitfssd_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstVaitfssd *vaitfssd = GST_VAITFSSD (filter);

  GST_DEBUG_OBJECT (vaitfssd, "set_info");

  return TRUE;
}

/* transform */
static GstFlowReturn
gst_vaitfssd_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{
  GstVaitfssd *vaitfssd = GST_VAITFSSD (filter);

  GST_DEBUG_OBJECT (vaitfssd, "transform_frame");

  return GST_FLOW_OK;
}

    static GstFlowReturn
    gst_vaitfssd_transform_frame_ip (GstVideoFilter * filter, GstVideoFrame * frame)
    {
      GstVaitfssd *vaitfssd = GST_VAITFSSD (filter);

      /* Create ssd detection object */
      thread_local auto ssd = vitis::ai::TFSSD::create("ssd_mobilenet_v1_coco_tf");

      /* Setup an OpenCV Mat with the frame data */
      cv::Mat img(360, 640, CV_8UC3, GST_VIDEO_FRAME_PLANE_DATA(frame, 0));

      /* Perform ssd detection */
      auto results = ssd->run(img);

      /* Draw bounding boxes */
      for (auto &box : results.bboxes)
      {
	class_label = classes[box.label];
        int xmin = box.x * img.cols;
        int ymin = box.y * img.rows;
        int xmax = xmin + (box.width * img.cols);
        int ymax = ymin + (box.height * img.rows);

        xmin = std::min(std::max(xmin, 0), img.cols);
        xmax = std::min(std::max(xmax, 0), img.cols);
        ymin = std::min(std::max(ymin, 0), img.rows);
        ymax = std::min(std::max(ymax, 0), img.rows);

        cv::rectangle(img, cv::Point(xmin, ymin), cv::Point(xmax, ymax),cv::Scalar(0, 255, 0), 2, 1, 0);
	cv::putText(img, class_label,cvPoint(xmin,ymin),cv::FONT_HERSHEY_SIMPLEX,1.0,cv::Scalar(0, 255, 0),1.0);
      }

      GST_DEBUG_OBJECT (vaitfssd, "transform_frame_ip");

      return GST_FLOW_OK;
    }

static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "vaitfssd", GST_RANK_NONE,
      GST_TYPE_VAITFSSD);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
    #ifndef VERSION
    #define VERSION "0.0.0"
    #endif
    #ifndef PACKAGE
    #define PACKAGE "vaitfssd"
    #endif
    #ifndef PACKAGE_NAME
    #define PACKAGE_NAME "GStreamer Xilinx Vitis-AI-Library"
    #endif
    #ifndef GST_PACKAGE_ORIGIN
    #define GST_PACKAGE_ORIGIN "http://xilinx.com"
    #endif

    GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        vaitfssd,
        "TFSSD using the Xilinx Vitis-AI-Library",
        plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)   
