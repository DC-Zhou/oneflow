#ifndef ONEFLOW_CORE_REGISTER_BLOB_H_
#define ONEFLOW_CORE_REGISTER_BLOB_H_

#include "oneflow/core/device/device_context.h"
#include "oneflow/core/job/resource.pb.h"
#include "oneflow/core/register/blob_desc.h"

namespace oneflow {

class Blob final {
 public:
  OF_DISALLOW_COPY_AND_MOVE(Blob);
  Blob(const BlobDesc* blob_desc, char* mem_ptr)
      : Blob(blob_desc, mem_ptr, nullptr) {}
  Blob(const BlobDesc* blob_desc, char* mem_ptr, const void* comm_net_token);
  ~Blob() = default;

  const char* data_id(int32_t no) const;
  char* mut_data_id(int32_t no) { return const_cast<char*>(data_id(no)); }

  const char* data_id() const { return data_id(0); }
  char* mut_data_id() { return mut_data_id(0); }

  const void* memory_ptr() const {
    return data_id_ptr_ == nullptr ? dptr_ : static_cast<void*>(data_id_ptr_);
  }
  void* mut_memory_ptr() { return const_cast<void*>(memory_ptr()); }

  template<typename T = void>
  const T* dptr() const {
    CheckDataType<T>();
    return static_cast<const T*>(dptr_);
  }

  template<typename T = void>
  T* mut_dptr() {
    CheckDataType<T>();
    return static_cast<T*>(dptr_);
  }

  const void* comm_net_token() const { return comm_net_token_; }

  const BlobDesc& blob_desc() const { return *blob_desc_; }
  const BlobDesc* blob_desc_ptr() const { return blob_desc_; }
  const Shape& shape() const { return blob_desc_->shape(); }
  DataType data_type() const { return blob_desc_->data_type(); }
  bool has_data_id() const { return blob_desc_->has_data_id(); }
  size_t ByteSizeOfDataIdField() const {
    return blob_desc_->ByteSizeOfDataIdField();
  }
  size_t ByteSizeOfDataContentField() const {
    return blob_desc_->ByteSizeOfDataContentField();
  }
  size_t TotalByteSize() const { return blob_desc_->TotalByteSize(); }

  template<DeviceType device_type>
  void CopyDataContentFrom(DeviceCtx* device_ctx, const Blob* rhs);
  template<DeviceType device_type>
  void CopyDataIdFrom(DeviceCtx* device_ctx, const Blob* rhs);
  template<DeviceType device_type>
  void CopyFrom(DeviceCtx* device_ctx, const Blob* rhs);

 private:
  template<typename T>
  void CheckDataType() const {
    LOG_IF(FATAL, (std::is_same<T, void>::value == false
                   && std::is_same<T, char>::value == false
                   && blob_desc_->data_type() != DataType::kChar
                   && blob_desc_->data_type() != GetDataType<T>::val))
        << blob_desc_->data_type() << " " << GetDataType<T>::val;
  }

  char* data_id_ptr_;
  void* dptr_;
  const void* comm_net_token_;
  const BlobDesc* blob_desc_;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_REGISTER_BLOB_H_
