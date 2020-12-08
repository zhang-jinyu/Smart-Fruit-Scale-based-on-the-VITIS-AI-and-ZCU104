/* GStreamer
 * Copyright (C) 2019 Seungha Yang <seungha.yang@navercorp.com>
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

#include <gst/check/gstcheck.h>

static gboolean
bus_cb (GstBus * bus, GstMessage * message, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *debug = NULL;

      gst_message_parse_error (message, &err, &debug);

      GST_ERROR ("Error: %s : %s", err->message, debug);
      g_error_free (err);
      g_free (debug);

      fail_if (TRUE, "failed");
      g_main_loop_quit (loop);
    }
      break;
    case GST_MESSAGE_EOS:
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }
  return TRUE;
}

static void
run_convert_pipelne (const gchar * in_format, const gchar * out_format)
{
  GstBus *bus;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  gchar *pipeline_str =
      g_strdup_printf ("videotestsrc num-buffers=1 is-live=true ! "
      "video/x-raw,format=%s,framerate=3/1 ! cudaupload ! "
      "cudaconvert ! cudadownload ! video/x-raw,format=%s ! "
      "videoconvert ! autovideosink", in_format, out_format);
  GstElement *pipeline;

  pipeline = gst_parse_launch (pipeline_str, NULL);
  fail_unless (pipeline != NULL);
  g_free (pipeline_str);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, (GstBusFunc) bus_cb, loop);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_bus_remove_watch (bus);
  gst_object_unref (bus);
  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
}

GST_START_TEST (test_convert_yuv_yuv)
{
  const gchar *format_list[] = {
    "I420", "YV12", "NV12", "NV21", "P010_10LE", "I420_10LE",
    "Y444", "Y444_16LE",
  };

  gint i, j;

  for (i = 0; i < G_N_ELEMENTS (format_list); i++) {
    for (j = 0; j < G_N_ELEMENTS (format_list); j++) {
      if (i == j)
        continue;

      GST_DEBUG ("run conversion %s to %s", format_list[i], format_list[j]);
      run_convert_pipelne (format_list[i], format_list[j]);
    }
  }
}

GST_END_TEST;

GST_START_TEST (test_convert_yuv_rgb)
{
  const gchar *in_format_list[] = {
    "I420", "YV12", "NV12", "NV21", "P010_10LE", "I420_10LE",
    "Y444", "Y444_16LE",
  };
  const gchar *out_format_list[] = {
    "BGRA", "RGBA", "RGBx", "BGRx", "ARGB", "ABGR", "RGB", "BGR", "BGR10A2_LE",
    "RGB10A2_LE",
  };

  gint i, j;

  for (i = 0; i < G_N_ELEMENTS (in_format_list); i++) {
    for (j = 0; j < G_N_ELEMENTS (out_format_list); j++) {
      GST_DEBUG ("run conversion %s to %s", in_format_list[i],
          out_format_list[j]);
      run_convert_pipelne (in_format_list[i], out_format_list[j]);
    }
  }
}

GST_END_TEST;

GST_START_TEST (test_convert_rgb_yuv)
{
  const gchar *in_format_list[] = {
    "BGRA", "RGBA", "RGBx", "BGRx", "ARGB", "ABGR", "RGB", "BGR", "BGR10A2_LE",
    "RGB10A2_LE",
  };
  const gchar *out_format_list[] = {
    "I420", "YV12", "NV12", "NV21", "P010_10LE", "I420_10LE",
    "Y444", "Y444_16LE",
  };

  gint i, j;

  for (i = 0; i < G_N_ELEMENTS (in_format_list); i++) {
    for (j = 0; j < G_N_ELEMENTS (out_format_list); j++) {
      GST_DEBUG ("run conversion %s to %s", in_format_list[i],
          out_format_list[j]);
      run_convert_pipelne (in_format_list[i], out_format_list[j]);
    }
  }
}

GST_END_TEST;

GST_START_TEST (test_convert_rgb_rgb)
{
  const gchar *format_list[] = {
    "BGRA", "RGBA", "RGBx", "BGRx", "ARGB", "ABGR", "RGB", "BGR", "BGR10A2_LE",
    "RGB10A2_LE",
  };

  gint i, j;

  for (i = 0; i < G_N_ELEMENTS (format_list); i++) {
    for (j = 0; j < G_N_ELEMENTS (format_list); j++) {
      if (i == j)
        continue;

      GST_DEBUG ("run conversion %s to %s", format_list[i], format_list[j]);
      run_convert_pipelne (format_list[i], format_list[j]);
    }
  }
}

GST_END_TEST;

static gboolean
check_cuda_convert_available (void)
{
  gboolean ret = TRUE;
  GstElement *upload;

  upload = gst_element_factory_make ("cudaconvert", NULL);
  if (!upload) {
    GST_WARNING ("cudaconvert is not available, possibly driver load failure");
    return FALSE;
  }

  gst_object_unref (upload);

  return ret;
}

static Suite *
cudaconvert_suite (void)
{
  Suite *s;
  TCase *tc_chain;

  /* HACK: cuda device init/deinit with fork seems to problematic */
  g_setenv ("CK_FORK", "no", TRUE);

  s = suite_create ("cudaconvert");
  tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  if (!check_cuda_convert_available ()) {
    GST_DEBUG ("Skip cudaconvert test since cannot open device");
    goto end;
  }

  tcase_add_test (tc_chain, test_convert_yuv_yuv);
  tcase_add_test (tc_chain, test_convert_yuv_rgb);
  tcase_add_test (tc_chain, test_convert_rgb_yuv);
  tcase_add_test (tc_chain, test_convert_rgb_rgb);

end:
  return s;
}

GST_CHECK_MAIN (cudaconvert);
