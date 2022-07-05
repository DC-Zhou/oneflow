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
#ifndef ONEFLOW_CORE_FRAMEWORK_STREAM_NEED_SOFT_SYNC_H_
#define ONEFLOW_CORE_FRAMEWORK_STREAM_NEED_SOFT_SYNC_H_

#include <glog/logging.h>
#include "oneflow/core/common/device_type.h"
#include "oneflow/core/common/stream_role.h"

namespace oneflow {

struct NeedSoftSync : public StreamRoleVisitor<NeedSoftSync> {
  static bool VisitCompute(DeviceType device_type) { return device_type != kCPU; }
  static bool VisitHost2Device(DeviceType) { return false; }
  static bool VisitDevice2Host(DeviceType) { return false; }
  static bool VisitSyncedLaunchedCommNet(DeviceType device_type) { return false; }
  static bool VisitAsyncedLaunchedCommNet(DeviceType) { return false; }
  static bool VisitBarrier(DeviceType) { return false; }
  static bool VisitCriticalSection(DeviceType) { return false; }
  static bool VisitLazyJobLauncher(DeviceType) { return false; }
  static bool VisitPinnedCompute(DeviceType device_type) { return VisitCompute(device_type); }
  static bool VisitTmpCompute(DeviceType device_type) { return VisitCompute(device_type); }
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_FRAMEWORK_STREAM_NEED_SOFT_SYNC_H_
