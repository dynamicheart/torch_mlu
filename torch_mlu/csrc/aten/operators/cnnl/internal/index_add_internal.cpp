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

void cnnl_index_add_internal(
    const at::Tensor& output,
    const at::Tensor& input,
    int64_t dim,
    const at::Tensor& index,
    const at::Tensor& source) {
  auto input_impl = getMluTensorImpl(input);
  auto index_impl = getMluTensorImpl(index);
  auto source_impl = getMluTensorImpl(source);
  auto output_impl = getMluTensorImpl(output);

  // get current handle
  auto handle = getCurrentHandle();
  CnnlTensorDescriptor desc_input;
  CnnlTensorDescriptor desc_index;
  CnnlTensorDescriptor desc_source;
  CnnlTensorDescriptor desc_output;

  // get cnnl descriptor
  desc_input.set(input);
  desc_index.set(index);
  desc_source.set(source);
  desc_output.set(output);

  auto input_ptr = input_impl->cnnlMalloc();
  auto index_ptr = index_impl->cnnlMalloc();
  auto source_ptr = source_impl->cnnlMalloc();
  auto output_ptr = output_impl->cnnlMalloc();

  TORCH_CNNL_CHECK(cnnlIndexAdd(
      handle,
      dim,
      desc_input.desc(),
      input_ptr,
      desc_index.desc(),
      index_ptr,
      desc_source.desc(),
      source_ptr,
      desc_output.desc(),
      output_ptr));
  // return output;
}

} // namespace ops
} // namespace torch_mlu
