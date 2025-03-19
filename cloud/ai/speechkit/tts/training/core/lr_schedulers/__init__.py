import torch.optim as optim

from .lr_schedulers import (
    LinearWarmupLR,
    LinearWarmupMilestonesLR,
    LinearWarmupCosineAnnealingLR
)


def create_scheduler(config: dict, optimizer: optim.Optimizer) -> optim.lr_scheduler._LRScheduler:
    schedulers = {
        "linear_warmup": LinearWarmupLR,
        "linear_warmup_cosine_annealing": LinearWarmupCosineAnnealingLR,
        "linear_warmup_milestones": LinearWarmupMilestonesLR
    }
    scheduler_args = {key: value for key, value in config.items() if key != "name"}
    return schedulers[config["name"]](optimizer, **scheduler_args)
