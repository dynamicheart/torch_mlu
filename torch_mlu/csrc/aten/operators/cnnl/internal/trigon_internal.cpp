#include "aten/operators/cnnl/internal/cnnl_internal.h"

namespace torch_mlu {
namespace ops {

at::Tensor& cnnl_trigon_internal(
    at::Tensor& output,
    const at::Tensor& self,
    cnnlTrigonFunctionMode_t mode) {
  if (self.numel() == 0) {
    return output;
  }

  auto input_impl = getMluTensorImpl(self);
  auto output_impl = getMluTensorImpl(output);

  // get current handle
  auto handle = getCurrentHandle();
  CnnlTensorDescriptor input_desc;
  CnnlTensorDescriptor output_desc;
  CnnlTrigonDescriptor trigon_desc;
  input_desc.set(self, CNNL_LAYOUT_ARRAY);
  output_desc.set(output, CNNL_LAYOUT_ARRAY);
  trigon_desc.set(mode, CNNL_COMPUTATION_HIGH_PRECISION);

  // malloc mlu memory
  auto input_ptr = input_impl->mlu_data_ptr();
  auto output_ptr = output_impl->mlu_data_ptr();

  // set descriptor config
  if (mode == CNNL_TRIGON_COS) {
    TORCH_CNNL_CHECK(cnnlCos_v2(
        handle,
        CNNL_COMPUTATION_HIGH_PRECISION,
        input_desc.desc(),
        input_ptr,
        output_desc.desc(),
        output_ptr));
  } else if (mode == CNNL_TRIGON_SIN) {
    TORCH_CNNL_CHECK(cnnlSin_v2(
        handle,
        CNNL_COMPUTATION_HIGH_PRECISION,
        input_desc.desc(),
        input_ptr,
        output_desc.desc(),
        output_ptr));
  } else {
    TORCH_CNNL_CHECK(cnnlTrigonForward(
        handle,
        trigon_desc.desc(),
        input_desc.desc(),
        input_ptr,
        output_desc.desc(),
        output_ptr));
  }
  return output;
}

at::Tensor& cnnl_atan2_internal(
    at::Tensor& output,
    const at::Tensor& input,
    const at::Tensor& other) {
  auto input_impl = getMluTensorImpl(input);
  auto other_impl = getMluTensorImpl(other);
  auto output_impl = getMluTensorImpl(output);

  // get current handle
  auto handle = getCurrentHandle();
  CnnlTensorDescriptor input_desc;
  CnnlTensorDescriptor other_desc;
  CnnlTensorDescriptor output_desc;

  input_desc.set(input, CNNL_LAYOUT_ARRAY);
  other_desc.set(other, CNNL_LAYOUT_ARRAY);
  output_desc.set(output, CNNL_LAYOUT_ARRAY);

  // malloc mlu memory
  auto input_ptr = input_impl->mlu_data_ptr();
  auto other_ptr = other_impl->mlu_data_ptr();
  auto output_ptr = output_impl->mlu_data_ptr();

  cnnlComputationPreference_t prefer = CNNL_COMPUTATION_HIGH_PRECISION;

  // get the size of workspace for brodcast
  size_t workspace_size = 0;
  TORCH_CNNL_CHECK(cnnlGetAtan2WorkspaceSize(
      handle,
      input_desc.desc(),
      other_desc.desc(),
      output_desc.desc(),
      &workspace_size));

  auto workspace_ptr =
      torch_mlu::MLUCachingAllocator::get()->allocate(workspace_size);
  // set descriptor config
  TORCH_CNNL_CHECK(cnnlAtan2(
      handle,
      prefer,
      input_desc.desc(),
      input_ptr,
      other_desc.desc(),
      other_ptr,
      workspace_ptr.get(),
      workspace_size,
      output_desc.desc(),
      output_ptr));
  return output;
}

} // namespace ops
} // namespace torch_mlu
