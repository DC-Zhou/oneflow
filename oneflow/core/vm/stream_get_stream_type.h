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
#ifndef ONEFLOW_CORE_VM_STREAM_GET_STREAM_TYPE_H_
#define ONEFLOW_CORE_VM_STREAM_GET_STREAM_TYPE_H_

#include "oneflow/core/common/stream_role.h"
#include "oneflow/core/common/singleton_ptr.h"
#include "oneflow/core/vm/async_cuda_stream_type.h"
#include "oneflow/core/vm/control_stream_type.h"
#include "oneflow/core/vm/cpu_stream_type.h"
#include "oneflow/core/vm/critical_section_stream_type.h"
#include "oneflow/core/vm/cuda_copy_d2h_stream_type.h"
#include "oneflow/core/vm/cuda_copy_h2d_stream_type.h"
#include "oneflow/core/vm/cuda_stream_type.h"
#include "oneflow/core/vm/lazy_job_stream_type.h"
#include "oneflow/core/vm/stream_get_stream_type.h"

namespace oneflow {

struct GetStreamType {
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kInvalid>,
                                           DeviceType device_type) {  // NOLINT
    UNIMPLEMENTED_THEN_RETURN();
  }
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kCompute>,
                                           DeviceType device_type) {
    if (device_type == DeviceType::kCPU) {
      return SingletonPtr<vm::CpuStreamType>();
    } else if (device_type == DeviceType::kCUDA) {
      return SingletonPtr<vm::CudaStreamType>();
    } else {
      UNIMPLEMENTED_THEN_RETURN();
    }
  }
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kHost2Device>,
                                           DeviceType device_type) {
    if (device_type == DeviceType::kCUDA) {
      return SingletonPtr<vm::CudaCopyH2DStreamType>();
    } else {
      UNIMPLEMENTED_THEN_RETURN();
    }
  }
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kDevice2Host>,
                                           DeviceType device_type) {
    if (device_type == DeviceType::kCUDA) {
      return SingletonPtr<vm::CudaCopyD2HStreamType>();
    } else {
      UNIMPLEMENTED_THEN_RETURN();
    }
  }
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kSyncedLaunchedCommNet>,
                                           DeviceType device_type) {
    if (device_type == DeviceType::kCPU) {
      return SingletonPtr<vm::CpuStreamType>();
    } else if (device_type == DeviceType::kCUDA) {
      return SingletonPtr<vm::CudaStreamType>();
    } else {
      UNIMPLEMENTED_THEN_RETURN();
    }
  }
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kAsyncedLaunchedCommNet>,
                                           DeviceType device_type) {
    if (device_type == DeviceType::kCPU) {
      return SingletonPtr<vm::CpuStreamType>();
    } else if (device_type == DeviceType::kCUDA) {
      return SingletonPtr<vm::AsyncCudaStreamType>();
    } else {
      UNIMPLEMENTED_THEN_RETURN();
    }
  }
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kBarrier>,
                                           DeviceType device_type) {
    return SingletonPtr<vm::ControlStreamType>();
  }
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kCriticalSection>,
                                           DeviceType device_type) {
    return SingletonPtr<vm::CriticalSectionStreamType>();
  }
  static Maybe<const vm::StreamType*> Case(StreamRoleCase<StreamRole::kLazyJobLauncher>,
                                           DeviceType device_type) {
    return SingletonPtr<vm::LazyJobStreamType>();
  }
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_VM_STREAM_GET_STREAM_TYPE_H_
