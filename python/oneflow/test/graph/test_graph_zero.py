"""
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
"""
import unittest
import os
import numpy as np

import oneflow as flow
import oneflow.unittest


def _test_linear_train_graph_with_zero(test_case, zero_stage=1):
    def train_with_graph(iter_num=1):
        P = flow.placement("cuda", ranks=[0, 1])
        B = flow.sbp.broadcast
        S0 = flow.sbp.split(0)

        linear_dp = flow.nn.Linear(8, 4, bias=False)
        linear_dp = linear_dp.to_global(placement=P, sbp=B)
        flow.nn.init.constant_(linear_dp.weight, 2.068758)

        linear_mp = flow.nn.Linear(4, 5, bias=False)
        linear_mp = linear_mp.to_global(placement=P, sbp=S0)
        flow.nn.init.constant_(linear_mp.weight, 2.068758)

        of_sgd = flow.optim.SGD([{"params": linear_dp.parameters()}, {"params": linear_mp.parameters()}], lr=0.001, momentum=0.9)
        grad_scaler = flow.amp.StaticGradScaler(200)

        x = flow.randint(1, 100, (6, 8), dtype=flow.float32, placement=P, sbp=S0)

        class LinearTrainGraphWithZeRO(flow.nn.Graph):
            def __init__(self):
                super().__init__()
                self.linear_dp = linear_dp
                self.linear_mp = linear_mp
                self.add_optimizer(of_sgd)

                self.config.enable_amp(True)
                self.set_grad_scaler(grad_scaler)
                self.config.enable_zero(True, stage=zero_stage, min_splited_size=1)
                self.debug(2)

            def build(self, x):
                out = self.linear_dp(x)
                out = out.to_global(placement=P, sbp=B)
                out = self.linear_mp(out)
                loss = out.sum()
                loss.backward()
                return out

        class LinearEvalGraphWithZeRO(flow.nn.Graph):
            def __init__(self):
                super().__init__()
                self.linear_dp = linear_dp
                self.linear_mp = linear_mp

                self.config.enable_amp(True)

            def build(self, x):
                out = self.linear_dp(x)
                out = out.to_global(placement=P, sbp=B)
                out = self.linear_mp(out)
                return out

        linear_t_g = LinearTrainGraphWithZeRO()
        linear_t_g.debug(1)
        linear_e_g = LinearEvalGraphWithZeRO()
        linear_e_g.debug(1)

        def one_train_iter():
            out = linear_t_g(x)

        def one_eval_iter():
            out = linear_e_g(x)

        for i in range(iter_num):
            one_train_iter()

        # After pass rewrite in training graph, parameters' sbp has been
        # changed from flow.sbp.broadcast to flow.sbp.split(0)
        test_case.assertEqual(linear_dp.weight.sbp[0], S0)
        test_case.assertEqual(linear_mp.weight.sbp[0], S0)

        # In evaluation graph, paramters's sbp are flow.sbp.split(0).
        # But their consumer will consum them as flow.sbp.broadcast.
        one_eval_iter()

    iter_num = 1
    graph_check_list = train_with_graph(iter_num)


@unittest.skipIf(os.getenv("ONEFLOW_TEST_CPU_ONLY"), "only test cpu cases")
@flow.unittest.skip_unless_1n2d()
class TestLinearTrainGraphWithZeRO(oneflow.unittest.TestCase):
    def test_linear_train_graph_with_zero_1(test_case):
        _test_linear_train_graph_with_zero(test_case, 1)

    def _test_linear_train_graph_with_zero_2(test_case):
        _test_linear_train_graph_with_zero(test_case, 2)

    def _test_linear_train_graph_with_zero_3(test_case):
        _test_linear_train_graph_with_zero(test_case, 3)


if __name__ == "__main__":
    unittest.main()
