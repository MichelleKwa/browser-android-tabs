// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_VAAPI_VAAPI_JPEG_ENCODER_H_
#define MEDIA_GPU_VAAPI_VAAPI_JPEG_ENCODER_H_

#include <va/va.h>
#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "media/gpu/media_gpu_export.h"
#include "ui/gfx/geometry/size.h"

namespace media {

class VaapiWrapper;

// A collection of methods that utilize VA-API hardware video encode
// acceleration on Intel systems. Provides functionality to allow plugging VAAPI
// HW acceleration into the JpegEncodeAccelerator framework.
//
// Clients are expected to manage VA surfaces and VA buffers created via
// VaapiWrapper, and pass them to this class.
class MEDIA_GPU_EXPORT VaapiJpegEncoder {
 public:
  // |vaapi_wrapper| should be initialized in VaapiWrapper::kEncode
  // mode with VAProfileJPEGBaseline profile.
  explicit VaapiJpegEncoder(scoped_refptr<VaapiWrapper> vaapi_wrapper);
  ~VaapiJpegEncoder();

  // Encode a JPEG picture. It will fill VA-API parameters and call
  // corresponding VA-API methods according to |input_size|.
  // |quality| is the JPEG image quality
  // |surface_id| is the VA surface that contains input image.
  // |output_buffer_id| is the ID of VA buffer that encoded image will be
  // stored. The size of it should be at least as large as
  // GetMaxCodedBufferSize().
  // Return false on failure.
  bool Encode(const gfx::Size& input_size,
              int quality,
              VASurfaceID surface_id,
              VABufferID output_buffer_id);

  // Gets the maximum possible encoded result size.
  // |size| is the dimension of the YUV image to be encoded.
  static size_t GetMaxCodedBufferSize(const gfx::Size& size);

 private:
  scoped_refptr<VaapiWrapper> vaapi_wrapper_;

  // |q_matrix_cached_|, |huff_table_param_cached_| and |slice_param_cached_|
  // are created when Encode() is called the first time. After that, they will
  // directly be used for all the subsequent Encode() calls.
  std::unique_ptr<VAQMatrixBufferJPEG> q_matrix_cached_;
  std::unique_ptr<VAHuffmanTableBufferJPEGBaseline> huff_table_param_cached_;
  std::unique_ptr<VAEncSliceParameterBufferJPEG> slice_param_cached_;

  DISALLOW_COPY_AND_ASSIGN(VaapiJpegEncoder);
};

}  // namespace media

#endif  // MEDIA_GPU_VAAPI_VAAPI_JPEG_ENCODER_H_
