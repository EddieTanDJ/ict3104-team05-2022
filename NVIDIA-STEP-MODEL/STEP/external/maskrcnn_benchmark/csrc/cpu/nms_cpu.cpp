// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
/*
* MIT License
* 
* Copyright (c) 2018 Facebook
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "cpu/vision.h"


template <typename scalar_t>
at::Tensor nms_cpu_kernel(const at::Tensor& dets,
                          const at::Tensor& scores,
                          const float threshold) {
  AT_ASSERTM(!dets.type().is_cuda(), "dets must be a CPU tensor");
  AT_ASSERTM(!scores.type().is_cuda(), "scores must be a CPU tensor");
  AT_ASSERTM(dets.type() == scores.type(), "dets should have the same type as scores");

  if (dets.numel() == 0) {
    return at::empty({0}, dets.options().dtype(at::kLong).device(at::kCPU));
  }

  auto x1_t = dets.select(1, 0).contiguous();
  auto y1_t = dets.select(1, 1).contiguous();
  auto x2_t = dets.select(1, 2).contiguous();
  auto y2_t = dets.select(1, 3).contiguous();

  at::Tensor areas_t = (x2_t - x1_t + 1) * (y2_t - y1_t + 1);

  auto order_t = std::get<1>(scores.sort(0, /* descending=*/true));

  auto ndets = dets.size(0);
  at::Tensor suppressed_t = at::zeros({ndets}, dets.options().dtype(at::kByte).device(at::kCPU));

  auto suppressed = suppressed_t.data<uint8_t>();
  auto order = order_t.data<int64_t>();
  auto x1 = x1_t.data<scalar_t>();
  auto y1 = y1_t.data<scalar_t>();
  auto x2 = x2_t.data<scalar_t>();
  auto y2 = y2_t.data<scalar_t>();
  auto areas = areas_t.data<scalar_t>();

  for (int64_t _i = 0; _i < ndets; _i++) {
    auto i = order[_i];
    if (suppressed[i] == 1)
      continue;
    auto ix1 = x1[i];
    auto iy1 = y1[i];
    auto ix2 = x2[i];
    auto iy2 = y2[i];
    auto iarea = areas[i];

    for (int64_t _j = _i + 1; _j < ndets; _j++) {
      auto j = order[_j];
      if (suppressed[j] == 1)
        continue;
      auto xx1 = std::max(ix1, x1[j]);
      auto yy1 = std::max(iy1, y1[j]);
      auto xx2 = std::min(ix2, x2[j]);
      auto yy2 = std::min(iy2, y2[j]);

      auto w = std::max(static_cast<scalar_t>(0), xx2 - xx1 + 1);
      auto h = std::max(static_cast<scalar_t>(0), yy2 - yy1 + 1);
      auto inter = w * h;
      auto ovr = inter / (iarea + areas[j] - inter);
      if (ovr >= threshold)
        suppressed[j] = 1;
   }
  }
  return at::nonzero(suppressed_t == 0).squeeze(1);
}

at::Tensor nms_cpu(const at::Tensor& dets,
               const at::Tensor& scores,
               const float threshold) {
  at::Tensor result;
  AT_DISPATCH_FLOATING_TYPES(dets.type(), "nms", [&] {
    result = nms_cpu_kernel<scalar_t>(dets, scores, threshold);
  });
  return result;
}
