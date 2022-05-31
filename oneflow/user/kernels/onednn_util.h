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
#ifndef ONEFLOW_USER_KERNELS_ONEDNN_UTIL_H_
#define ONEFLOW_USER_KERNELS_ONEDNN_UTIL_H_
#ifdef WITH_ONEDNN
#include <oneapi/dnnl/dnnl.hpp>
#include "oneflow/core/common/env_var/env_var.h"

namespace oneflow {
using dm = dnnl::memory;

template<typename T>
dnnl::memory::data_type CppTypeToOneDnnDtype();

DEFINE_ENV_BOOL(ONEFLOW_ENABLE_ONEDNN_OPTS, true);
inline bool OneDnnIsEnabled() { return EnvBool<ONEFLOW_ENABLE_ONEDNN_OPTS>(); }
}  // namespace oneflow
#endif  // WITH_ONEDNN
#endif  // ONEFLOW_USER_KERNELS_ONEDNN_UTIL_H_