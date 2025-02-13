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

/**
 * Note [beta and alpha type in matmul ops]
 *
 * For mm, bmm, addmm, addbmm, addmv, addr, baddbmm ops, beta and alpha type
 * for cnnl kernel is a little different with gpu side. GPU using float when
 * input type is float, half, BFloat16; and using double when input
 * type is double. MLU side always using float. And more specific CNNL kernel
 * names are cnnlBatchMatMulBCast_v2 and cnnlMatMul_v2.
 */

void cnnl_addmm_bias_out_internal(
    at::Tensor& result,
    const at::Tensor& self,
    const at::Tensor& mat1,
    const at::Tensor& mat2,
    bool is_trans_self_,
    bool is_trans_mat1_,
    bool is_trans_mat2_,
    const at::Scalar& beta_,
    const at::Scalar& alpha_,
    bool allow_tf32_) {
  // get tensor impl
  auto self_impl = getMluTensorImpl(self);
  auto mat1_impl = getMluTensorImpl(mat1);
  auto mat2_impl = getMluTensorImpl(mat2);
  auto result_impl = getMluTensorImpl(result);

  // create desc
  CnnlMatmulExDescriptor matmul_desc;
  CnnlMatmulExAlgorithm matmul_algo;
  cnnlMatMulExPrefer_t preference;
  CnnlMatmulExHeuristicResult matmul_hr;
  CnnlTensorDescriptor self_desc;
  CnnlTensorDescriptor mat1_desc;
  CnnlTensorDescriptor mat2_desc;
  CnnlTensorDescriptor result_desc;
  int return_algo_count;
  int requested_algo_count = 1;
  int32_t is_trans_self = is_trans_self_;
  int32_t is_trans_mat1 = is_trans_mat1_;
  int32_t is_trans_mat2 = is_trans_mat2_;
  int32_t allow_tf32 = allow_tf32_;

  matmul_desc.set_attr(
      CNNL_MATMUL_EX_ALLOW_TF32, &(allow_tf32), sizeof(int32_t));
  matmul_desc.set_attr(
      CNNL_MATMUL_EX_DESC_TRANSA, &(is_trans_mat1), sizeof(int32_t));
  matmul_desc.set_attr(
      CNNL_MATMUL_EX_DESC_TRANSB, &(is_trans_mat2), sizeof(int32_t));

  // set descriptor
  mat1_desc.set(mat1);
  mat2_desc.set(mat2);
  self_desc.set(self);
  result_desc.set(result);
  result_desc.set_onchip_dtype(CNNL_DTYPE_FLOAT);
  auto mat1_ptr = mat1_impl->mlu_data_ptr();
  auto mat2_ptr = mat2_impl->mlu_data_ptr();
  auto self_ptr = self_impl->mlu_data_ptr();
  auto result_ptr = result_impl->mlu_data_ptr();

  // bias => self*beta(1.0) => self
  cnnlMatMulEpilogueType_t epilogue_type = CNNL_MATMUL_EPI_BIAS;
  matmul_desc.set_attr(
      CNNL_MATMUL_EX_DESC_EPILOGUE_TYPE,
      &(epilogue_type),
      sizeof(epilogue_type));
  TORCH_CNNL_CHECK(
      cnnlSetMatMulExBias(matmul_desc.desc(), self_desc.desc(), self_ptr));

  auto handle = getCurrentHandle();
  matmul_hr.get(
      handle,
      matmul_desc.desc(),
      mat1_desc.desc(),
      mat2_desc.desc(),
      NULL,
      result_desc.desc(),
      preference,
      requested_algo_count,
      &return_algo_count);

  size_t workspace_size = 0;
  TORCH_CNNL_CHECK(cnnlGetMatMulExHeuristicResult(
      matmul_hr.hr(), matmul_algo.mut_algo(), &workspace_size));
  auto workspace_ptr =
      torch_mlu::MLUCachingAllocator::get()->allocate(workspace_size);

  // bf16, complex are not supported
  AT_DISPATCH_FLOATING_TYPES_AND2(
      at::ScalarType::Half,
      at::ScalarType::BFloat16,
      self.scalar_type(),
      "MLU mm",
      [&] {
        // More details in Note [beta and alpha type in matmul ops]
        using math_type = MLUAccumulateType_t<scalar_t>;
        auto alpha = alpha_.to<math_type>();
        auto beta = beta_.to<math_type>();
        // Not use beta, set beta to 0.0
        beta = 0.0;
        TORCH_CNNL_CHECK(cnnlMatMulEx(
            handle,
            matmul_desc.desc(),
            &alpha,
            mat1_desc.desc(),
            mat1_ptr,
            mat2_desc.desc(),
            mat2_ptr,
            &beta,
            NULL,
            NULL,
            result_desc.desc(),
            result_ptr,
            matmul_algo.algo(),
            workspace_ptr.get(),
            workspace_size));
      });
}

} // namespace ops
} // namespace torch_mlu
