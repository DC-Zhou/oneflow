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
#include "oneflow/core/common/util.h"
#include "oneflow/core/job_rewriter/job_pass.h"
#include "oneflow/core/framework/framework.h"

namespace oneflow {

struct SGDOptimizerKey {
  std::string learning_rate;
  std::string scale_by_tensor_lbn;
  std::string skip_if_lbn;
  double scale;
  float l1;
  float l2;
  float weight_decay;
  ParallelConf parallel_conf;
};

bool operator==(const SGDOptimizerKey& lhs, const SGDOptimizerKey& rhs) {
  return (lhs.learning_rate == rhs.learning_rate)
         && (lhs.scale_by_tensor_lbn == rhs.scale_by_tensor_lbn)
         && (lhs.skip_if_lbn == rhs.skip_if_lbn) && (lhs.scale == rhs.scale) && (lhs.l1 == rhs.l1)
         && (lhs.l2 == rhs.l2) && (lhs.weight_decay == rhs.weight_decay)
         && (lhs.parallel_conf == rhs.parallel_conf);
}

struct AdamOptimizerKey {
  std::string learning_rate;
  std::string scale_by_tensor_lbn;
  std::string skip_if_lbn;
  std::string bias_correction1_lbn;
  std::string bias_correction2_lbn;
  double scale;
  float l1;
  float l2;
  float beta1;
  float beta2;
  float epsilon;
  float weight_decay;
  bool amsgrad;
  bool do_bias_correction;
  ParallelConf parallel_conf;
};

bool operator==(const AdamOptimizerKey& lhs, const AdamOptimizerKey& rhs) {
  return (lhs.learning_rate == rhs.learning_rate)
         && (lhs.scale_by_tensor_lbn == rhs.scale_by_tensor_lbn)
         && (lhs.skip_if_lbn == rhs.skip_if_lbn)
         && (lhs.bias_correction1_lbn == rhs.bias_correction1_lbn)
         && (lhs.bias_correction2_lbn == rhs.bias_correction2_lbn) && (lhs.scale == rhs.scale)
         && (lhs.l1 == rhs.l1) && (lhs.l2 == rhs.l2) && (lhs.beta1 == rhs.beta1)
         && (lhs.beta2 == rhs.beta2) && (lhs.epsilon == rhs.epsilon)
         && (lhs.weight_decay == rhs.weight_decay) && (lhs.amsgrad == rhs.amsgrad)
         && (lhs.do_bias_correction == rhs.do_bias_correction)
         && (lhs.parallel_conf == rhs.parallel_conf);
}

}  // namespace oneflow

namespace std {

template<>
struct hash<oneflow::SGDOptimizerKey> {
  size_t operator()(const oneflow::SGDOptimizerKey& key) const {
    const auto& float_hash = std::hash<float>();
    const auto& double_hash = std::hash<float>();
    const auto& string_hash = std::hash<std::string>();
    const auto& parallel_conf_hash = std::hash<oneflow::ParallelConf>();

    return string_hash(key.learning_rate) ^ string_hash(key.scale_by_tensor_lbn)
           ^ string_hash(key.skip_if_lbn) ^ double_hash(key.scale) ^ float_hash(key.l1)
           ^ float_hash(key.l2) ^ float_hash(key.weight_decay)
           ^ parallel_conf_hash(key.parallel_conf);
  }
};

template<>
struct hash<oneflow::AdamOptimizerKey> {
  size_t operator()(const oneflow::AdamOptimizerKey& key) const {
    const auto& float_hash = std::hash<float>();
    const auto& double_hash = std::hash<float>();
    const auto& string_hash = std::hash<std::string>();
    const auto& bool_hash = std::hash<bool>();
    const auto& parallel_conf_hash = std::hash<oneflow::ParallelConf>();

    return string_hash(key.learning_rate) ^ string_hash(key.scale_by_tensor_lbn)
           ^ string_hash(key.skip_if_lbn) ^ string_hash(key.bias_correction1_lbn)
           ^ string_hash(key.bias_correction2_lbn) ^ double_hash(key.scale) ^ float_hash(key.l1)
           ^ float_hash(key.l2) ^ float_hash(key.beta1) ^ float_hash(key.beta2)
           ^ float_hash(key.epsilon) ^ float_hash(key.weight_decay) ^ bool_hash(key.amsgrad)
           ^ bool_hash(key.do_bias_correction) ^ parallel_conf_hash(key.parallel_conf);
  }
};

}  // namespace std

namespace oneflow {

namespace {

bool IsUserOpWithTypeName(const OperatorConf& op_conf, const std::string& op_type_name) {
  return op_conf.has_user_conf() && op_conf.user_conf().op_type_name() == op_type_name;
};

void AddScaleAndSkipLbn(user_op::UserOpConfWrapperBuilder& multi_tensor_op_builder,
                        const user_op::UserOpConfWrapper& model_update_user_conf) {
  if (model_update_user_conf.has_input("scale_by_tensor", 0)) {
    multi_tensor_op_builder.Input("scale_by_tensor", model_update_user_conf.input("scale_by_tensor", 0));
  }
  if (model_update_user_conf.has_input("skip_if", 0)) {
    multi_tensor_op_builder.Input("skip_if", model_update_user_conf.input("skip_if", 0));
  }
}

class MultiTensorUpdatePass final : public JobPass {
 public:
  MultiTensorUpdatePass() = default;
  ~MultiTensorUpdatePass() override = default;

  bool IsEnabled(const JobPassCtx& ctx) const {
    return ParseBooleanFromEnv("ONEFLOW_ENABLE_MULTI_TENSOR_UPDATE", false);
  }
  Maybe<void> Apply(const OpGraph& op_graph, JobBuilder* job_builder) const;

  Maybe<void> Apply(Job* job, JobPassCtx* ctx) const override {
    if (!IsEnabled(*ctx)) { return Maybe<void>::Ok(); }
    const OpGraph op_graph(*job);
    JobBuilder job_builder(job);
    return Apply(op_graph, &job_builder);
  }
};

Maybe<void> MultiTensorUpdatePass::Apply(const OpGraph& op_graph, JobBuilder* job_builder) const {
  if (!job_builder->job().job_conf().has_train_conf()) { return Maybe<void>::Ok(); }
  std::vector<OperatorConf> delete_ops;
  ParallelConf parallel_conf{};
  HashMap<SGDOptimizerKey, user_op::UserOpConfWrapperBuilder> multi_tensor_sgd_update_hashmap;
  HashMap<AdamOptimizerKey, user_op::UserOpConfWrapperBuilder> multi_tensor_adam_update_hashmap;

  op_graph.ForEachNode([&](OpNode* op_node) {
    const auto& op_conf = op_node->op().op_conf();
    if (!op_conf.has_variable_conf()) { return; }
    LogicalBlobId model_half_lbi;

    for (OpEdge* find_model_update_edge : op_node->out_edges()) {
      OpNode* find_model_update_update_node = find_model_update_edge->dst_node();
      if (!IsUserOpWithTypeName(find_model_update_update_node->op().op_conf(), "sgd_update")
          && !IsUserOpWithTypeName(find_model_update_update_node->op().op_conf(), "adam_update")) {
        continue;
      }
      const user_op::UserOpConfWrapper model_update_user_conf(
          find_model_update_update_node->op().op_conf());
      // Multi tensor update pass only support for CUDA currently.
      if (find_model_update_update_node->parallel_desc().device_type() != DeviceType::kCUDA) {
        continue;
      }

      delete_ops.emplace_back(find_model_update_update_node->op().op_conf());
      parallel_conf = find_model_update_update_node->parallel_desc().parallel_conf();

      std::string scale_by_tensor_lbn = "";
      std::string skip_if_lbn = "";
      if (model_update_user_conf.has_input("scale_by_tensor", 0)) {
        scale_by_tensor_lbn = model_update_user_conf.input("scale_by_tensor", 0);
      }
      if (model_update_user_conf.has_input("skip_if", 0)) {
        skip_if_lbn = model_update_user_conf.input("skip_if", 0);
      }
      if (IsUserOpWithTypeName(find_model_update_update_node->op().op_conf(), "sgd_update")) {
        SGDOptimizerKey key{model_update_user_conf.input("learning_rate", 0),
                            scale_by_tensor_lbn,
                            skip_if_lbn,
                            model_update_user_conf.attr<double>("scale"),
                            model_update_user_conf.attr<float>("l1"),
                            model_update_user_conf.attr<float>("l2"),
                            model_update_user_conf.attr<float>("weight_decay"),
                            parallel_conf};
        const auto& iter = multi_tensor_sgd_update_hashmap.find(key);

        if (iter != multi_tensor_sgd_update_hashmap.end()) {
          iter->second.Input("model", model_update_user_conf.input("model", 0))
              .Input("model_diff", model_update_user_conf.input("model_diff", 0));
        } else {
          user_op::UserOpConfWrapperBuilder multi_tensor_update_sgd_op_builder(
              "multi_tensor_update");
          std::string op_type_name = "multi_tensor_sgd_update"; 
          if(model_update_user_conf.has_input("model_half", 0)){
            op_type_name = "multi_tensor_sgd_update_with_cast"; 
          }
          multi_tensor_update_sgd_op_builder.OpTypeName(op_type_name)
              .Input("model", model_update_user_conf.input("model", 0))
              .Input("model_diff", model_update_user_conf.input("model_diff", 0))
              .Input("learning_rate", model_update_user_conf.input("learning_rate", 0))
              .Attr<double>("scale", model_update_user_conf.attr<double>("scale"))
              .Attr<float>("l1", model_update_user_conf.attr<float>("l1"))
              .Attr<float>("l2", model_update_user_conf.attr<float>("l2"))
              .Attr<float>("weight_decay", model_update_user_conf.attr<float>("weight_decay"));
          if(model_update_user_conf.has_input("model_half", 0)){
            multi_tensor_update_sgd_op_builder.Input("model_half", model_update_user_conf.input("model_half", 0)); 
          }
          AddScaleAndSkipLbn(multi_tensor_update_sgd_op_builder, model_update_user_conf);

          CHECK(model_update_user_conf.op_conf().has_scope_symbol_id());
          multi_tensor_update_sgd_op_builder.ScopeSymbolId(
              model_update_user_conf.op_conf().scope_symbol_id());
          multi_tensor_sgd_update_hashmap.emplace(key, multi_tensor_update_sgd_op_builder);
        }
      } else if (IsUserOpWithTypeName(find_model_update_update_node->op().op_conf(),
                                      "adam_update")) {
        AdamOptimizerKey key{model_update_user_conf.input("learning_rate", 0),
                             scale_by_tensor_lbn,
                             skip_if_lbn,
                             model_update_user_conf.input("bias_correction1", 0),
                             model_update_user_conf.input("bias_correction1", 0),
                             model_update_user_conf.attr<double>("scale"),
                             model_update_user_conf.attr<float>("l1"),
                             model_update_user_conf.attr<float>("l2"),
                             model_update_user_conf.attr<float>("beta1"),
                             model_update_user_conf.attr<float>("beta2"),
                             model_update_user_conf.attr<float>("epsilon"),
                             model_update_user_conf.attr<float>("weight_decay"),
                             model_update_user_conf.attr<bool>("amsgrad"),
                             model_update_user_conf.attr<bool>("do_bias_correction"),
                             parallel_conf};
        if (key.amsgrad) {
          UNIMPLEMENTED() << "Multi Tensor Adam update do not support amsgrad = True. ";
        }
        const auto& iter = multi_tensor_adam_update_hashmap.find(key);

        if (iter != multi_tensor_adam_update_hashmap.end()) {
          iter->second.Input("model", model_update_user_conf.input("model", 0))
              .Input("model_diff", model_update_user_conf.input("model_diff", 0))
              .Input("m", model_update_user_conf.input("m", 0))
              .Input("v", model_update_user_conf.input("v", 0));
        } else {
          user_op::UserOpConfWrapperBuilder multi_tensor_update_adam_op_builder(
              "multi_tensor_update");
          multi_tensor_update_adam_op_builder.OpTypeName("multi_tensor_adam_update")
              .Input("model", model_update_user_conf.input("model", 0))
              .Input("model_diff", model_update_user_conf.input("model_diff", 0))
              .Input("m", model_update_user_conf.input("m", 0))
              .Input("v", model_update_user_conf.input("v", 0))
              .Input("learning_rate", model_update_user_conf.input("learning_rate", 0))
              .Attr<double>("scale", model_update_user_conf.attr<double>("scale"))
              .Attr<float>("l1", model_update_user_conf.attr<float>("l1"))
              .Attr<float>("l2", model_update_user_conf.attr<float>("l2"))
              .Attr<float>("beta1", model_update_user_conf.attr<float>("beta1"))
              .Attr<float>("beta2", model_update_user_conf.attr<float>("beta2"))
              .Attr<float>("epsilon", model_update_user_conf.attr<float>("epsilon"))
              .Attr<float>("weight_decay", model_update_user_conf.attr<float>("weight_decay"))
              .Attr<bool>("amsgrad", model_update_user_conf.attr<bool>("amsgrad"))
              .Attr<bool>("do_bias_correction",
                          model_update_user_conf.attr<bool>("do_bias_correction"));

          if (model_update_user_conf.attr<bool>("do_bias_correction")) {
            multi_tensor_update_adam_op_builder
                .Input("bias_correction1", model_update_user_conf.input("bias_correction1", 0))
                .Input("bias_correction2", model_update_user_conf.input("bias_correction2", 0));
          }
          AddScaleAndSkipLbn(multi_tensor_update_adam_op_builder, model_update_user_conf);

          CHECK(model_update_user_conf.op_conf().has_scope_symbol_id());
          multi_tensor_update_adam_op_builder.ScopeSymbolId(
              model_update_user_conf.op_conf().scope_symbol_id());
          multi_tensor_adam_update_hashmap.emplace(key, multi_tensor_update_adam_op_builder);
        }
      } else {
        UNIMPLEMENTED() << "Current Optimizer do not support multi tensor update. ";
      }
      break;
    }
  });
  for (auto& op : multi_tensor_sgd_update_hashmap) {
    auto multi_tensor_update_sgd_op = op.second.Build();
    job_builder->AddOps(parallel_conf, {multi_tensor_update_sgd_op.op_conf()});
  }
  for (auto& op : multi_tensor_adam_update_hashmap) {
    auto multi_tensor_update_adam_op = op.second.Build();
    job_builder->AddOps(parallel_conf, {multi_tensor_update_adam_op.op_conf()});
  }
  job_builder->DelOps(delete_ops);
  printf("Success pass \n"); 
  return Maybe<void>::Ok();
}

}  // namespace

REGISTER_JOB_PASS("MultiTensorUpdatePass", MultiTensorUpdatePass);

}  // namespace oneflow
