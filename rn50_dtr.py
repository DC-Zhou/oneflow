# coding=utf-8

import time
import os

import numpy as np
from numpy import random
from tqdm import tqdm
from torch.utils.tensorboard import SummaryWriter


# resnet50 bs 32, use_disjoint_set=False: threshold ~800MB

# memory policy:
# 1: only reuse the memory block with exactly the same size
# 2: reuse the memory block with the same size or larger

# os.environ["OF_DTR"] = "1"
# os.environ["OF_DTR_THRESHOLD"] = "3500mb"
# os.environ["OF_DTR_DEBUG"] = "0"
# os.environ["OF_DTR_LR"] = "1"
# os.environ["OF_DTR_BS"] = "80"
# os.environ["OF_ITERS"] = "40"

import argparse


class NegativeArgAction(argparse.Action):
    def __init__(self, option_strings, dest, env_var_name, **kwargs):
        assert len(option_strings) == 1
        assert "--no-" in option_strings[0]
        dest = dest[3:]
        super(NegativeArgAction, self).__init__(
            option_strings, dest, nargs=0, default=True, **kwargs
        )
        self.env_var_name = env_var_name
        os.environ[self.env_var_name] = "True"

    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, False)
        os.environ[self.env_var_name] = "False"


class PositiveArgAction(argparse.Action):
    def __init__(self, option_strings, dest, env_var_name, **kwargs):
        super(PositiveArgAction, self).__init__(
            option_strings, dest, nargs=0, default=True, **kwargs
        )
        self.env_var_name = env_var_name
        os.environ[self.env_var_name] = "False"

    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, False)
        os.environ[self.env_var_name] = "True"


# Set random seed for reproducibility.
def setup_seed(seed):
    random.seed(seed)
    os.environ["PYTHONHASHSEED"] = str(seed)
    np.random.seed(seed)
    flow.manual_seed(seed)


parser = argparse.ArgumentParser()
parser.add_argument("bs", type=int)
parser.add_argument("threshold", type=str)
parser.add_argument("iters", type=int)
parser.add_argument("exp_id", type=str)
parser.add_argument("--no-dtr", action=NegativeArgAction, env_var_name="OF_DTR")
parser.add_argument("--no-lr", action=NegativeArgAction, env_var_name="OF_DTR_LR")
parser.add_argument("--no-o-one", action=NegativeArgAction, env_var_name="OF_DTR_O_ONE")
parser.add_argument("--no-ee", action=NegativeArgAction, env_var_name="OF_DTR_EE")
parser.add_argument(
    "--no-allocator", action=NegativeArgAction, env_var_name="OF_DTR_ALLO"
)
parser.add_argument("--nlr", action=PositiveArgAction, env_var_name="OF_DTR_NLR")
parser.add_argument(
    "--high-conv", action=PositiveArgAction, env_var_name="OF_DTR_HIGH_CONV"
)
parser.add_argument(
    "--high-add-n", action=PositiveArgAction, env_var_name="OF_DTR_HIGH_ADD_N"
)
parser.add_argument("--debug-level", type=int, default=0)
parser.add_argument("--no-dataloader", action='store_true')

args = parser.parse_args()

# print(os.environ)

import oneflow as flow
import oneflow.nn as nn
import flowvision
import flowvision.transforms as transforms
import flowvision.models as models

# import resnet50_model
# import resnet50

# run forward, backward and update parameters
WARMUP_ITERS = 5
ALL_ITERS = args.iters

# NOTE: it has not effect for dtr allocator
heuristic = "eq"

if args.dtr:
    print(
        f"dtr_enabled: {args.dtr}, dtr_allo: {args.allocator}, threshold: {args.threshold}, batch size: {args.bs}, eager eviction: {args.ee}, left and right: {args.lr}, debug_level: {args.debug_level}, heuristic: {heuristic}, o_one: {args.o_one}"
    )
else:
    print(f"dtr_enabled: {args.dtr}")

if args.dtr:
    flow.enable_dtr(args.dtr, args.threshold, args.debug_level, heuristic)

seed = 20
setup_seed(seed)

writer = SummaryWriter("./tensorboard/" + args.exp_id)


def display():
    flow._oneflow_internal.dtr.display()


model = models.resnet50()

weights = flow.load("/tmp/abcdef")
model.load_state_dict(weights)

criterion = nn.CrossEntropyLoss()

cuda0 = flow.device("cuda:0")

model.to(cuda0)

criterion.to(cuda0)

learning_rate = 0.1
optimizer = flow.optim.SGD(model.parameters(), lr=learning_rate, momentum=0.9, weight_decay=1e-4)

normalize = transforms.Normalize(mean=[0.485, 0.456, 0.406],
                                 std=[0.229, 0.224, 0.225])

if args.no_dataloader:
    global data
    global label
    data = flow.ones(args.bs, 3, 224, 224).to('cuda')
    label = flow.ones(args.bs, dtype=flow.int64).to('cuda')

    class FixedDataset(flow.utils.data.Dataset):
        def __len__(self):
            return 999999999

        def __getitem__(self, idx):
            return data, label

    train_data_loader = FixedDataset()
else:
    imagenet_data = flowvision.datasets.ImageFolder(
        "/dataset/imagenet_folder/train",
        transforms.Compose(
            [
                transforms.RandomResizedCrop(224),
                transforms.RandomHorizontalFlip(),
                transforms.ToTensor(),
                normalize,
            ]
        ),
    )
    train_data_loader = flow.utils.data.DataLoader(
        imagenet_data, batch_size=args.bs, shuffle=True, num_workers=0
    )

total_time = 0
# train_bar = tqdm(train_data_loader, dynamic_ncols=True)

for iter, (train_data, train_label) in enumerate(train_data_loader):
# for iter in range(10000):
    # train_data = data
    # train_label = label
    if args.dtr:
        for x in model.parameters():
            x.grad = flow.zeros_like(x).to(cuda0)

        flow.comm.barrier()
        # temp()

    train_data = train_data.to(cuda0)
    train_label = train_label.to(cuda0)

    if iter >= WARMUP_ITERS:
        start_time = time.time()

    logits = model(train_data)
    loss = criterion(logits, train_label)
    del logits
    loss.backward()
    # train_bar.set_description(
    #     "Epoch {}: loss: {:.4f}".format(iter + 1, loss.item())
    # )
    # writer.add_scalar("Loss/train/loss", loss.item(), iter)
    # writer.flush()
    del loss

    optimizer.step()
    optimizer.zero_grad(True)

    flow.comm.barrier()

    if iter >= WARMUP_ITERS:
        end_time = time.time()
        this_time = end_time - start_time
        print(f'iter: {iter}, time: {this_time}')
        total_time += this_time
    print(f'iter {iter} end')

end_time = time.time()
print(
f"{ALL_ITERS - WARMUP_ITERS} iters: avg {(total_time) / (ALL_ITERS - WARMUP_ITERS)}s"
)

