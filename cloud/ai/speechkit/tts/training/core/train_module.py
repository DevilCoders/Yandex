import sys
from abc import ABC, abstractmethod
from typing import Dict, List, Optional, Tuple, Union

from core.optimizers import OptimizerWrapper
from core.utils import get_rank

import torch
from torch.optim import Optimizer
from torch.optim.lr_scheduler import _LRScheduler

_MetricTypes = Union[int, float, torch.Tensor]


class TrainModule(ABC, torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.trainer = None

    @abstractmethod
    def training_step(self, batch, batch_idx) -> Optional[Dict[str, _MetricTypes]]:
        pass

    @abstractmethod
    def validation_step(self, batch, batch_idx) -> Optional[Dict[str, _MetricTypes]]:
        pass

    @abstractmethod
    def configure_optimizers(self) -> Tuple[List[Optimizer], List[_LRScheduler]]:
        pass

    @abstractmethod
    def ddp(self):
        pass

    def save_hyperparameters(self) -> Optional[Dict]:
        pass

    def optimizers(self) -> Union[OptimizerWrapper, List[OptimizerWrapper]]:
        assert self.trainer is not None
        if len(self.trainer.optimizers) == 1:
            return self.trainer.optimizers[0]
        return self.trainer.optimizers

    def lr_schedulers(self) -> Union[_LRScheduler, List[_LRScheduler]]:
        assert self.trainer is not None
        if len(self.trainer.lr_schedulers) == 1:
            return self.trainer.lr_schedulers[0]
        return self.trainer.lr_schedulers

    def load_from_checkpoint(self, checkpoint_path: str, strict: bool = True):
        checkpoint = torch.load(checkpoint_path, map_location="cpu")
        self.load_state_dict(checkpoint["state_dict"], strict=strict)
        sys.stderr.write(f"#{get_rank()}: loaded state dict from {checkpoint_path}\n")
