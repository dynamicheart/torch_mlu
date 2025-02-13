/*
All modification made by Cambricon Corporation: © 2022 Cambricon Corporation
All rights reserved.
All other contributions:
Copyright (c) 2014--2022, the respective contributors
All rights reserved.
For the list of contributors go to
https://github.com/pytorch/pytorch/graphs/contributors Redistribution and use in
source and binary forms, with or without modification, are permitted provided
that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "aten/operators/cnnl/cnnl_kernel.h"
#include "aten/operators/cnnl/internal/cnnl_internal.h"

namespace torch_mlu {
namespace ops {

at::Tensor cnnl_nms(
    const at::Tensor& dets,
    const at::Tensor& scores,
    double iou_threshold) {
  auto dets_ = cnnl_contiguous(dets);
  auto scores_ = cnnl_contiguous(scores);

  TORCH_CHECK(
      dets_.dim() == 2, "boxes should be a 2d tensor, got ", dets_.dim(), "D");
  TORCH_CHECK(
      dets_.size(1) == 4,
      "boxes should have 4 elements in dimension 1, got ",
      dets_.size(1));
  TORCH_CHECK(
      scores_.dim() == 1,
      "scores should be a 1d tensor, got ",
      scores_.dim(),
      "D");
  TORCH_CHECK(
      dets_.size(0) == scores_.size(0),
      "boxes and scores should have same number of elements in ",
      "dimension 0, got ",
      dets_.size(0),
      " and ",
      scores_.size(0));

  TORCH_CHECK(dets_.device().is_privateuseone(), "dets must be a MLU tensor");
  TORCH_CHECK(
      scores_.device().is_privateuseone(), "scores must be a MLU tensor");
  TORCH_CHECK(
      dets_.scalar_type() == scores_.scalar_type(),
      "dets should have the same type as scores");

  return cnnl_nms_internal(dets_, scores_, iou_threshold);
}

} // namespace ops
} // namespace torch_mlu
