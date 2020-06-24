#include "oneflow/core/framework/framework.h"
#include "oneflow/customized/kernels/where_kernel_util.h"

namespace oneflow {

template<DeviceType device_type, typename T, typename CondT>
class WhereKernel final : public user_op::OpKernel {
 public:
  WhereKernel() = default;
  ~WhereKernel() = default;

 private:
  void Compute(user_op::KernelComputeContext* ctx) const override {
    const user_op::Tensor* cond = ctx->Tensor4ArgNameAndIndex("condition", 0);
    const user_op::Tensor* x = ctx->Tensor4ArgNameAndIndex("x", 0);
    const user_op::Tensor* y = ctx->Tensor4ArgNameAndIndex("y", 0);
    user_op::Tensor* out = ctx->Tensor4ArgNameAndIndex("out", 0);
    WhereKernelUtil<device_type, T, CondT>::Where(ctx->device_ctx(), out->shape().elem_cnt(),
                                                  cond->dptr<CondT>(), x->dptr<T>(), y->dptr<T>(),
                                                  out->mut_dptr<T>());
  }
  bool AlwaysComputeWhenAllOutputsEmpty() const override { return false; }
};

#define REGISTER_WHERE_KERNEL(device_type_v, dtype_pair, ctype_pair)                           \
  REGISTER_USER_KERNEL("where")                                                                \
      .SetCreateFn<WhereKernel<device_type_v, OF_PP_PAIR_FIRST(dtype_pair),                    \
                               OF_PP_PAIR_FIRST(ctype_pair)>>()                                \
      .SetIsMatchedHob(user_op::HobDeviceType() == device_type_v                               \
                       & user_op::HobDataType("condition", 0) == OF_PP_PAIR_SECOND(ctype_pair) \
                       & user_op::HobDataType("out", 0) == OF_PP_PAIR_SECOND(dtype_pair));

OF_PP_SEQ_PRODUCT_FOR_EACH_TUPLE(REGISTER_WHERE_KERNEL, DEVICE_TYPE_SEQ, ARITHMETIC_DATA_TYPE_SEQ,
                                 INT_DATA_TYPE_SEQ)

}  // namespace oneflow
