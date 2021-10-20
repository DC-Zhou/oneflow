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

#ifndef ONEFLOW_CORE_AUTOGRAD_AUTOGRAD_FUNCTION_H_
#define ONEFLOW_CORE_AUTOGRAD_AUTOGRAD_FUNCTION_H_

#include "oneflow/core/common/util.h"

namespace oneflow {
namespace one {

class TensorTuple;
class FunctionAutoGradCaptureState;

class AutogradFunctionBase {
 public:
  using FType = std::function<std::shared_ptr<TensorTuple>(
      const std::shared_ptr<FunctionAutoGradCaptureState>& ctx, const TensorTuple&)>;
  AutogradFunctionBase() = delete;
  virtual ~AutogradFunctionBase() = default;
  AutogradFunctionBase(const FType& forward_fn, const FType& backward_fn)
      : forward_fn_(forward_fn), backward_fn_(backward_fn) {}

  Maybe<TensorTuple> Apply(const TensorTuple& inputs) const;

 protected:
  FType forward_fn_;
  FType backward_fn_;
};

}  // namespace one
}  // namespace oneflow

#endif  // ONEFLOW_CORE_AUTOGRAD_AUTOGRAD_FUNCTION_H_
