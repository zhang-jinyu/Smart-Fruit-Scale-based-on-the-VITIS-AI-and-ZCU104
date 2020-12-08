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

#ifndef __GST_NVRTC_LOADER_H__
#define __GST_NVRTC_LOADER_H__

#include <gst/gst.h>
#include <nvrtc.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
gboolean gst_nvrtc_load_library (void);

G_GNUC_INTERNAL
nvrtcResult NvrtcCompileProgram (nvrtcProgram prog,
                                 int numOptions,
                                 const char** options);

G_GNUC_INTERNAL
nvrtcResult NvrtcCreateProgram  (nvrtcProgram* prog,
                                 const char* src,
                                 const char* name,
                                 int numHeaders,
                                 const char** headers,
                                 const char** includeNames);

G_GNUC_INTERNAL
nvrtcResult NvrtcDestroyProgram (nvrtcProgram* prog);

G_GNUC_INTERNAL
nvrtcResult NvrtcGetPTX         (nvrtcProgram prog,
                                 char* ptx);

G_GNUC_INTERNAL
nvrtcResult NvrtcGetPTXSize     (nvrtcProgram prog,
                                 size_t* ptxSizeRet);

G_GNUC_INTERNAL
nvrtcResult NvrtcGetProgramLog (nvrtcProgram prog,
                                char* log);

G_GNUC_INTERNAL
nvrtcResult NvrtcGetProgramLogSize (nvrtcProgram prog,
                                    size_t* logSizeRet);

G_END_DECLS
#endif /* __GST_NVRTC_LOADER_H__ */
