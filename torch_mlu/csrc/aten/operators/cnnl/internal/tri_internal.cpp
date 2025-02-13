#include "aten/operators/cnnl/internal/cnnl_internal.h"
#include "aten/utils/dispatch.h"
namespace torch_mlu {
namespace ops {

void cnnl_tri_internal(
    at::Tensor& output,
    const at::Tensor& input,
    int64_t diagonal,
    bool tri_up_mode) {
  auto handle = getCurrentHandle();
  auto input_impl = getMluTensorImpl(input);
  auto output_impl = getMluTensorImpl(output);
  CnnlTensorDescriptor desc_a;
  CnnlTensorDescriptor desc_b;
  desc_a.set(input);
  desc_b.set(output);
  auto input_ptr = input_impl->mlu_data_ptr();
  auto output_ptr = output_impl->mlu_data_ptr();
  // GPU not support BFloat16, CPU support BFloat16
  // Although CNNL kernel support BFloat16, CATCH don't add BFloat16 type
  // support. cnnl kernel only support int.
  int diagonal_int = static_cast<int>(diagonal);
  AT_DISPATCH_ALL_MLU_TYPES_AND_HALF_AND_BFLOAT16_EXCEPT_UINT8(
      input.scalar_type(), "tri_out_mlu_impl", [&] {
        TORCH_CNNL_CHECK(cnnlTri_v2(
            handle,
            diagonal_int,
            tri_up_mode,
            desc_a.desc(),
            input_ptr,
            desc_b.desc(),
            output_ptr));
      });
}

} // namespace ops
} // namespace torch_mlu
