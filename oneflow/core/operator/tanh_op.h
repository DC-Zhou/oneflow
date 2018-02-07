#ifndef ONEFLOW_CORE_OPERATOR_TANH_OP_H_
#define ONEFLOW_CORE_OPERATOR_TANH_OP_H_

#include "oneflow/core/operator/operator.h"

namespace oneflow {

class TanHOp final : public Operator {
 public:
  OF_DISALLOW_COPY_AND_MOVE(TanHOp);
  TanHOp() = default;
  ~TanHOp() = default;

  void InitFromOpConf() override;
  const PbMessage& GetSpecialConf() const override;
  bool IsElemWiseOp() const override { return true; }
  bool NeedExtraInDiffMemWhenBackward() const override { return false; }
  bool NeedOutWhenBackward() const override { return false; }

  void InferBlobDescs(
      std::function<BlobDesc*(const std::string)> GetBlobDesc4BnInOp,
      const ParallelContext* parallel_ctx) const override;

 private:
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_OPERATOR_TANH_OP_H_
