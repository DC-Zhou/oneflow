#include "oneflow/core/register/register.h"
#include "oneflow/core/job/keyword.h"
#include "oneflow/core/comm_network/comm_network.h"

namespace oneflow {

const std::vector<int64_t>& Regst::consumers_actor_id() const {
  return regst_desc_->consumers_actor_id();
}

Regst::Regst() {
  status_.regst_desc_id = -1;
  status_.piece_id = -1;
  status_.model_version_id = -1;
  status_.act_id = -1;
  status_.col_id = 0;
  status_.max_col_id = 0;
  regst_desc_ = nullptr;
  comm_net_token_ = nullptr;
}

Regst::~Regst() {
  if (comm_net_token_ != nullptr) { Global<CommNet>::Get()->UnRegisterMemory(comm_net_token_); }
}

Blob* Regst::GetBlobByLbi(const LogicalBlobId& lbi) {
  auto it = lbi2blob_.find(lbi);
  if (it != lbi2blob_.end()) {
    return static_cast<Blob*>(it->second.get());
  } else if (lbi.is_packed_id()) {
    return static_cast<Blob*>(packed_blob_.get());
  } else {
    return nullptr;
  }
}

}  // namespace oneflow
