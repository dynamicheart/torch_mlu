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

#include "aten/operators/cnnl/internal/cnnl_internal.h"

namespace torch_mlu {
namespace ops {

at::Tensor& cnnl_roll_internal(
    at::Tensor& output,
    const at::Tensor& input,
    at::IntArrayRef shifts,
    at::IntArrayRef dims) {
  std::vector<int64_t> v_shifts;
  for (auto v : shifts) {
    v_shifts.push_back(v);
  }

  std::vector<int64_t> v_dims;
  auto memory_format = input.suggest_memory_format();
  for (auto s : dims) {
    auto dim = modify_dim_based_on_layout(s, memory_format);
    v_dims.push_back(dim);
  }
  // get current handle
  auto handle = getCurrentHandle();

  CnnlTensorDescriptor desc_input;
  CnnlTensorDescriptor desc_output;
  desc_input.set(input);
  desc_output.set(output);

  size_t workspace_size = 0;
  TORCH_CNNL_CHECK(
      cnnlGetRollWorkspaceSize(handle, desc_input.desc(), &workspace_size));
  auto workspace_ptr =
      torch_mlu::MLUCachingAllocator::get()->allocate(workspace_size);
  auto input_ptr = getMluTensorImpl(input)->mlu_data_ptr();
  auto output_ptr = getMluTensorImpl(output)->mlu_data_ptr();

  AT_DISPATCH_ALL_TYPES_AND_COMPLEX_AND4(
      at::ScalarType::Half,
      at::ScalarType::Bool,
      at::ScalarType::BFloat16,
      at::ScalarType::ComplexHalf,
      input.scalar_type(),
      "MLU roll",
      [&] {
        TORCH_CNNL_CHECK(cnnlRoll_v2(
            handle,
            desc_input.desc(),
            input_ptr,
            v_shifts.data(),
            static_cast<int64_t>(v_shifts.size()),
            v_dims.data(),
            static_cast<int64_t>(v_dims.size()),
            workspace_ptr.get(),
            workspace_size,
            desc_output.desc(),
            output_ptr));
      });

  return output;
}

} // namespace ops
} // namespace torch_mlu
