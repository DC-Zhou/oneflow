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
#ifndef ONEFLOW_API_PYTHON_AST_TO_MLIR_API_
#define ONEFLOW_API_PYTHON_AST_TO_MLIR_API_

#include "oneflow/core/common/just.h"
#include "oneflow/core/common/maybe.h"
// #include <pybind11/pybind11.h>
// #include <pybind11/pytypes.h>

// namespace py = pybind11;

namespace oneflow {

void initAstToMLIRContext();
void finishAstToMLIRContext();

void emitMLIRConstantInt(int value);
void emitMLIRConstantFloat(float value);
void emitMLIRConstantBool(bool value);

}  // namespace oneflow

#endif