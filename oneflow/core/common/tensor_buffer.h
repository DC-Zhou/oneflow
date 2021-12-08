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
#ifndef ONEFLOW_CORE_COMMON_TENSOR_BUFFER_H_
#define ONEFLOW_CORE_COMMON_TENSOR_BUFFER_H_

#include "oneflow/core/common/util.h"
#include "oneflow/core/common/shape.h"
#include "oneflow/core/common/data_type.h"

namespace oneflow {

namespace detail {

class TensorBufferImpl;

}  // namespace detail

class TensorBuffer final {
 public:
  TensorBuffer() : impl_(nullptr) {}
  ~TensorBuffer();

  TensorBuffer(const Shape& shape, DataType dtype);

  TensorBuffer(const TensorBuffer&) = delete;
  TensorBuffer& operator=(const TensorBuffer&) = delete;

  TensorBuffer(TensorBuffer&& other) noexcept : impl_(other.impl_) { other.impl_ = nullptr; }
  TensorBuffer& operator=(TensorBuffer&& other) noexcept {
    impl_ = other.impl_;
    other.impl_ = nullptr;
    return *this;
  }

  bool is_allocated() const { return impl_ != nullptr; }
  const Shape& shape() const;
  DataType data_type() const;
  int64_t elem_cnt() const { return shape().elem_cnt(); }
  size_t nbytes() const { return elem_cnt() * GetSizeOfDataType(data_type()); }

  void Reset(const Shape& shape, DataType dtype);
  void Reset(const Shape& shape);
  void Reset(DataType dtype);
  void Reset();

  // backward compatible interface and will be deprecated in future
  void Resize(const Shape& shape, DataType dtype) { Reset(shape, dtype); }

  void CopyFrom(const TensorBuffer& src);
  void Swap(TensorBuffer* lhs);

  template<typename T = void>
  T* mut_data() {
    if (raw_data() == nullptr) { return nullptr; }
    CheckDataType<T>(data_type());
    return static_cast<T*>(raw_data());
  }

  template<typename T = void>
  const T* data() const {
    if (raw_data() == nullptr) { return nullptr; }
    CheckDataType<T>(data_type());
    return static_cast<const T*>(raw_data());
  }

 private:
  friend class TensorBufferPool;

  void* raw_data();
  const void* raw_data() const;

  detail::TensorBufferImpl* impl_;
};

#define BUFFER_DATA_TYPE_SEQ OF_PP_MAKE_TUPLE_SEQ(TensorBuffer, DataType::kTensorBuffer)

template<>
struct GetDataType<TensorBuffer> : std::integral_constant<DataType, DataType::kTensorBuffer> {};
inline TensorBuffer GetTypeByDataType(std::integral_constant<DataType, DataType::kTensorBuffer>) {
  return {};
}

class TensorBufferPool final {
 public:
  using TensorBufferList = std::vector<detail::TensorBufferImpl*>;

  static TensorBufferPool& Get() {
    CHECK(Ptr()) << "TensorBufferPool singleton instance does not exist, please use "
                    "TensorBufferPool::New(...) first.";
    return *Ptr().get();
  }

  static TensorBufferPool& New(size_t pool_size, size_t thread_local_cache_size) {
    CHECK(!Ptr()) << "TensorBufferPool singleton instance is already created.";
    auto* inst = new TensorBufferPool(pool_size, thread_local_cache_size);
    Ptr().reset(inst);
    return *inst;
  }

  static void Delete() {
    if (Ptr()) { Ptr().reset(); }
  }

  ~TensorBufferPool();
  OF_DISALLOW_COPY_AND_MOVE(TensorBufferPool);

  void Allocate(TensorBuffer* tensor_buffer, const Shape& shape, DataType dtype);
  void Deallocate(TensorBuffer* tensor_buffer);

 private:
  static std::unique_ptr<TensorBufferPool>& Ptr() {
    static std::unique_ptr<TensorBufferPool> ptr;
    return ptr;
  }

  TensorBufferPool(size_t pool_size, size_t thread_local_cache_size) noexcept
      : pool_size_(pool_size), thread_local_cache_size_(thread_local_cache_size) {}

  size_t pool_size_;
  size_t thread_local_cache_size_;

  TensorBufferList global_free_list_;
  std::mutex mtx_;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_COMMON_TENSOR_BUFFER_H_
