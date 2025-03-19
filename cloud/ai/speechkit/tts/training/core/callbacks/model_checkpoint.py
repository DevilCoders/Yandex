from collections import OrderedDict
import os

import torch

from core.callbacks.base import Callback
from core.optimizers import OptimizerWrapper
from core.utils import get_rank, rank_zero_only, rank_zero_info


class ModelCheckpoint(Callback):
    def __init__(self, checkpoint_dir: str, checkpoint_interval: int):
        super().__init__()
        self.checkpoint_interval = checkpoint_interval
        self.checkpoint_dir = checkpoint_dir

        if get_rank() == 0:
            os.makedirs(checkpoint_dir, exist_ok=True)

    @rank_zero_only
    def on_train_batch_end(
        self,
        trainer,
        train_module,
        batch,
        batch_idx: int
    ):
        if (batch_idx + 1) < self.checkpoint_interval or (batch_idx + 1) % self.checkpoint_interval != 0:
            return

        optimizers = train_module.optimizers()
        if isinstance(optimizers, list):
            optimizer_states = [optimizer.state_dict() for optimizer in optimizers]
        else:
            assert isinstance(optimizers, OptimizerWrapper)
            optimizer_states = [optimizers.state_dict()]

        lr_schedulers = train_module.lr_schedulers()
        if isinstance(lr_schedulers, list):
            lr_schedulers = [lr_scheduler.state_dict() for lr_scheduler in lr_schedulers if lr_scheduler is not None]
        else:
            lr_schedulers = [lr_schedulers.state_dict()] if lr_schedulers is not None else []

        state_dict = OrderedDict()
        for module_name, module in train_module._modules.items():
            module = module.module if hasattr(module, "module") else module
            for key, value in module.state_dict().items():
                state_dict[f"{module_name}.{key}"] = value

        dictionary = {
            "step": trainer.step,
            "state_dict": state_dict,
            "optimizer_states": optimizer_states,
            "lr_schedulers": lr_schedulers,
            "hparams": train_module.save_hyperparameters()
        }
        save_path = os.path.join(self.checkpoint_dir, "checkpoint.ckpt")
        torch.save(dictionary, save_path)

        rank_zero_info(f"saved model checkpoint")
