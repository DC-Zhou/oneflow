/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef ONEFLOW_CORE_EAGER_CALL_CONTEXT_H_
#define ONEFLOW_CORE_EAGER_CALL_CONTEXT_H_

#include "oneflow/core/framework/attr_map.h"
#include "oneflow/core/eager/eager_blob_object.h"
#include "oneflow/core/framework/op_interpreter.h"
#include "oneflow/core/common/shape_view.h"

namespace oneflow {

namespace one {

class StatefulLocalOpKernel;
class ConsistentTensorInferResult;

using EagerBlobObjectList = std::vector<std::shared_ptr<vm::EagerBlobObject>>;
using EagerBlobObjectListPtr =
    std::shared_ptr<const std::vector<std::shared_ptr<vm::EagerBlobObject>>>;

}  // namespace one

class DeviceCtx;

namespace eager {

struct CallContext {
  CallContext(
      ComposedAttrMap&& composed_attrs, const one::EagerBlobObjectListPtr& inputs,
      const one::EagerBlobObjectListPtr& outputs,
      const std::shared_ptr<const one::ConsistentTensorInferResult>& consistent_tensor_infer_result,
      const one::OpExprInterpContext& op_interp_ctx,
      const std::shared_ptr<one::StatefulLocalOpKernel> opkernel)
      : composed_attrs(composed_attrs),
        inputs(inputs),
        outputs(outputs),
        consistent_tensor_infer_result(consistent_tensor_infer_result),
        op_interp_ctx(op_interp_ctx),
        opkernel(opkernel),
        tmp_buffer_ptr(nullptr),
        tmp_buffer_size(0),
        shape_view(&tmp_buffer_size, 1),
        mut_shape_view(&tmp_buffer_size, 1),
        device_ctx(nullptr),
        state(nullptr),
        cache(nullptr) {}

  ComposedAttrMap composed_attrs;
  one::EagerBlobObjectListPtr inputs;
  one::EagerBlobObjectListPtr outputs;
  std::shared_ptr<const one::ConsistentTensorInferResult> consistent_tensor_infer_result;
  const one::OpExprInterpContext op_interp_ctx;
  const std::shared_ptr<one::StatefulLocalOpKernel> opkernel;
  char* tmp_buffer_ptr;
  int64_t tmp_buffer_size;
  ShapeView shape_view;
  MutShapeView mut_shape_view;
  DeviceCtx* device_ctx;
  user_op::OpKernelState* state;
  user_op::OpKernelCache* cache;
};

class ThreadLocalCallContextScope final {
 public:
  ThreadLocalCallContextScope(CallContext* call_ctx) {
    CHECK_ISNULL(*MutCurrent());
    *MutCurrent() = call_ctx;
  }
  ~ThreadLocalCallContextScope() {
    CHECK_NOTNULL(*MutCurrent());
    *MutCurrent() = nullptr;
  }

  static CallContext* Current() { return CHECK_NOTNULL(*MutCurrent()); }
  static bool CurrentIsValid() { return *MutCurrent() != nullptr; }

 private:
  static CallContext** MutCurrent() {
    static thread_local CallContext* call_ctx = nullptr;
    return &call_ctx;
  }
};

}  // namespace eager

}  // namespace oneflow

#endif  // ONEFLOW_CORE_EAGER_CALL_CONTEXT_H_
