/*
All modification made by Cambricon Corporation: © 2023 Cambricon Corporation
All rights reserved.
All other contributions:
Copyright (c) 2014--2023, the respective contributors
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

std::tuple<at::Tensor, at::Tensor> cnnl__weight_norm_interface(
    const at::Tensor& v,
    const at::Tensor& g,
    int64_t dim) {
  auto w = at::empty_like(v, at::MemoryFormat::Contiguous);
  at::ScalarType AccType = g.scalar_type() == at::ScalarType::Half
      ? at::ScalarType::Float
      : g.scalar_type();
  auto norms =
      at::empty_strided(g.sizes(), g.strides(), g.options().dtype(AccType));
  auto v_contiguous = cnnl_contiguous(v, at::MemoryFormat::Contiguous);
  auto g_contiguous = cnnl_contiguous(g, at::MemoryFormat::Contiguous);

  AT_DISPATCH_FLOATING_TYPES_AND_HALF(
      v.scalar_type(), "cnnl__weight_norm_interface", [&] {
        cnnl_weight_norm_internal(w, norms, v_contiguous, g_contiguous, dim);
      });
  return std::tuple<at::Tensor, at::Tensor>{w, norms};
}

} // namespace ops
} // namespace torch_mlu
