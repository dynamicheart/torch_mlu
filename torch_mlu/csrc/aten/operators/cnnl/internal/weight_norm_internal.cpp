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

#include "aten/operators/cnnl/internal/cnnl_internal.h"

namespace torch_mlu {
namespace ops {

void cnnl_weight_norm_internal(
    at::Tensor& w,
    at::Tensor& norms,
    const at::Tensor& v,
    const at::Tensor& g,
    const int64_t dim) {
  // [in] handle
  auto handle = getCurrentHandle();

  // [in] axis
  const int axis = static_cast<int>(dim);

  // [in] v_desc & [in] v
  CnnlTensorDescriptor v_desc;
  v_desc.set(v);
  auto v_impl = getMluTensorImpl(v);
  auto v_ptr = v_impl->mlu_data_ptr();

  // [in] g_desc & [in] g
  CnnlTensorDescriptor g_desc;
  g_desc.set(g);
  auto g_impl = getMluTensorImpl(g);
  auto g_ptr = g_impl->mlu_data_ptr();

  // [in] w_desc & [out] w
  CnnlTensorDescriptor w_desc;
  w_desc.set(w);
  auto w_impl = getMluTensorImpl(w);
  auto w_ptr = w_impl->mlu_data_ptr();

  // [in] norm_recip_desc & [out] norm_recip
  CnnlTensorDescriptor norm_recip_desc;
  norm_recip_desc.set(norms);
  auto norm_recip_impl = getMluTensorImpl(norms);
  auto norm_recip_ptr = norm_recip_impl->mlu_data_ptr();

  // WeightNorm operation
  TORCH_CNNL_CHECK(cnnlWeightNorm(
      /*handle         */ handle,
      /*axis           */ axis,
      /*v_desc         */ v_desc.desc(),
      /*v              */ v_ptr,
      /*g_desc         */ g_desc.desc(),
      /*g              */ g_ptr,
      /*w_desc         */ w_desc.desc(),
      /*w              */ w_ptr,
      /*norm_recip_desc*/ norm_recip_desc.desc(),
      /*norm_recip     */ norm_recip_ptr));

  return;
}

} // namespace ops
} // namespace torch_mlu
