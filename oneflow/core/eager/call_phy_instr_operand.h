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
#ifndef ONEFLOW_CORE_EAGER_CALL_PHY_INSTR_OPERAND_H_
#define ONEFLOW_CORE_EAGER_CALL_PHY_INSTR_OPERAND_H_

#include "oneflow/core/vm/phy_instr_operand.h"
#include "oneflow/core/eager/call_context.h"
#include "oneflow/core/eager/dev_vm_dep_object_consume_mode.h"

namespace oneflow {

namespace user_op {

class OpKernel;

}  // namespace user_op

namespace vm {

class CallPhyInstrOperand final : public vm::PhyInstrOperand {
 public:
  CallPhyInstrOperand(const CallPhyInstrOperand&) = delete;
  CallPhyInstrOperand(CallPhyInstrOperand&&) = delete;
  ~CallPhyInstrOperand() override = default;

  // recursive scope.
  template<typename CallbackT>
  auto WithThisCallContext(const CallbackT& Callback) -> decltype(Callback()) {
    if (eager::ThreadLocalCallContextScope::CurrentIsValid()) {
      CHECK_EQ(eager::ThreadLocalCallContextScope::Current(), &call_ctx_);
      return decltype(Callback())();
    }
    eager::ThreadLocalCallContextScope scope(&call_ctx_);
    return Callback();
  }

  template<typename... Args>
  static Maybe<CallPhyInstrOperand> New(Args&&... args) {
    auto* ptr = new CallPhyInstrOperand(std::forward<Args>(args)...);
    JUST(ptr->Init());
    return std::shared_ptr<CallPhyInstrOperand>(ptr);
  }

  const one::StatefulLocalOpKernel& opkernel() const { return *opkernel_; }
  const one::EagerBlobObjectListPtr& inputs() const { return call_ctx_.inputs; }
  const one::EagerBlobObjectListPtr& outputs() const { return call_ctx_.outputs; }
  const AttrMap& attrs() const { return call_ctx_.op_interp_ctx.attrs; }
  const one::OpExprInterpContext& op_interp_ctx() const { return call_ctx_.op_interp_ctx; }
  const one::DevVmDepObjectConsumeMode& dev_vm_dep_object_consume_mode() const {
    return dev_vm_dep_object_consume_mode_;
  }

  one::StatefulLocalOpKernel* mut_opkernel() { return opkernel_.get(); }

  template<typename DoEachT>
  Maybe<void> ForEachOutputTensor(const DoEachT& DoEach) {
    for (const auto& output : *outputs()) { JUST(DoEach(output.get())); }
    return Maybe<void>::Ok();
  }

  const DependenceVector& input_dependences() const override { return input_dependences_; }
  const DependenceVector& output_dependences() const override { return output_dependences_; }

  void ForEachConstMirroredObject(const std::function<void(vm::MirroredObject* compute)>&) const;

  void ForEachMutMirroredObject(const std::function<void(vm::MirroredObject* compute)>&) const;

  void ForEachMut2MirroredObject(const std::function<void(vm::MirroredObject* compute)>&) const;

  bool need_temp_storage() const { return need_temp_storage_; }
  const user_op::OpKernel* user_opkernel() const { return user_opkernel_; }

  const std::shared_ptr<const one::ConsistentTensorInferResult>& consistent_tensor_infer_result()
      const {
    return call_ctx_.consistent_tensor_infer_result;
  }

 private:
  CallPhyInstrOperand(
      const std::shared_ptr<one::StatefulLocalOpKernel>& opkernel,
      const one::EagerBlobObjectListPtr& inputs, const one::EagerBlobObjectListPtr& outputs,
      const std::shared_ptr<const one::ConsistentTensorInferResult>& consistent_tensor_infer_result,
      const one::OpExprInterpContext& op_interp_ctx,
      const one::DevVmDepObjectConsumeMode dev_vm_dep_object_consume_mode);

  Maybe<void> Init();
  void InitStreamSequentialDependence();

  eager::CallContext call_ctx_;
  std::shared_ptr<one::StatefulLocalOpKernel> opkernel_;
  const user_op::OpKernel* user_opkernel_;
  bool need_temp_storage_;
  const one::DevVmDepObjectConsumeMode dev_vm_dep_object_consume_mode_;
  DependenceVector input_dependences_;
  DependenceVector output_dependences_;
};

}  // namespace vm
}  // namespace oneflow

#endif  // ONEFLOW_CORE_EAGER_CALL_PHY_INSTR_OPERAND_H_
