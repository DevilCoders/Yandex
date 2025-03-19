from typing import Iterator

import torch.optim as optim
from torch.nn.parameter import Parameter

from .wrapper import OptimizerWrapper


def create_optimizer(config: dict, parameters: Iterator[Parameter]) -> optim.Optimizer:
    optimizers = {
        "Adam": optim.Adam,
        "AdamW": optim.AdamW,
    }
    optimizer_args = {key: value for key, value in config.items() if key not in {"name", "scheduler"}}
    return optimizers[config["name"]](parameters, **optimizer_args)
