from typing import Dict, Optional

from core.loggers.base import Logger
from core.utils import get_rank, rank_zero_only

import torch
from torch.utils.tensorboard import SummaryWriter


class TensorBoardLogger(Logger):
    def __init__(self, log_dir: str):
        super().__init__()
        self.log_dir = log_dir
        self._summary_writer = SummaryWriter(log_dir) if get_rank() == 0 else None

    @rank_zero_only
    def log_metrics(self, metrics: Dict[str, float], step: Optional[int] = None):
        assert get_rank() == 0

        for k, v in metrics.items():
            if isinstance(v, torch.Tensor):
                v = v.item()

            if isinstance(v, dict):
                self._summary_writer.add_scalars(k, v, step)
            else:
                try:
                    self._summary_writer.add_scalar(k, v, step)
                # todo: specify the possible exception
                except Exception as ex:
                    m = f"\n you tried to log {v} which is currently not supported. Try a dict or a scalar/tensor."
                    raise ValueError(m) from ex

    @rank_zero_only
    def finalize(self):
        self._summary_writer.flush()
        self._summary_writer.close()
