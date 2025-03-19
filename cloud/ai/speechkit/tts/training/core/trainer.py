from collections import defaultdict
import sys
from typing import Dict, List, Optional, Sequence, Tuple, Union

from core.callbacks import Callback, CallbackCollection
from core.data import TtsDataModule
from core.loggers import DummyLogger, Logger, LoggerCollection
from core.optimizers import OptimizerWrapper
from core.train_module import TrainModule
from core.utils import get_rank

import torch
import torch.distributed as dist
from torch.optim import Optimizer

_MetricTypes = Union[int, float, torch.Tensor]


class Trainer:
    def __init__(
        self,
        max_steps: int,
        log_every_n_steps: int,
        val_check_interval: int,
        logger: Optional[Union[Logger, Sequence[Logger]]] = None,
        callbacks: Optional[Union[Callback, List[Callback]]] = None,
        gradient_clip_val: Optional[float] = None,
        gradient_clip_algorithm: Optional[str] = None,
        device: str = "cpu",
        use_amp: bool = False
    ):
        self.max_steps = max_steps
        self.log_every_n_steps = log_every_n_steps
        self.val_check_interval = val_check_interval
        if logger is None:
            self.logger = DummyLogger()
        else:
            self.logger = LoggerCollection(logger) if isinstance(logger, list) else logger
        if callbacks is None:
            self.callbacks = Callback()
        else:
            self.callbacks = CallbackCollection(callbacks) if isinstance(callbacks, list) else callbacks
        self.gradient_clip_val = gradient_clip_val
        self.gradient_clip_algorithm = gradient_clip_algorithm
        self.device = device
        self.use_amp = use_amp
        self.step = 0

        self.optimizers = None
        self.lr_schedulers = None

        self._metrics = {}
        self._val_metrics = {}

    def fit(
        self,
        model: TrainModule,
        data_module: TtsDataModule,
        ckpt_path: Optional[str] = None
    ):
        model.trainer = self
        self._init_optimizers(model)
        model.to(self.device)
        self._resume_from_checkpoint(model, ckpt_path)

        if dist.is_initialized():
            model.ddp()

        self.callbacks.on_train_start(self, model)
        for train_batch_idx, train_batch in enumerate(data_module.train_dataloader()):
            model.train()
            train_batch.to(self.device)

            self.step += 1
            if self.step > self.max_steps:
                break

            self.callbacks.on_train_batch_start(self, model, train_batch, train_batch_idx)
            metrics = model.training_step(train_batch, self.step)
            self.callbacks.on_train_batch_end(self, model, train_batch, train_batch_idx)

            if self.step % self.log_every_n_steps == 0:
                metrics = _metrics_to_scalars(metrics)
                self.logger.log_metrics(metrics, self.step)

            if self.step % self.val_check_interval == 0:
                model.eval()
                self.callbacks.on_validation_start(self, model)
                num_val_batches = 0
                val_metrics = defaultdict(int)
                val_dataloader = data_module.val_dataloader()
                if val_dataloader is None:
                    continue
                for val_batch_idx, val_batch in enumerate(val_dataloader):
                    val_batch.to(self.device)
                    self.callbacks.on_validation_batch_start(self, model, train_batch, val_batch_idx)
                    with torch.no_grad():
                        metrics = model.validation_step(val_batch, val_batch_idx)
                    self.callbacks.on_validation_batch_end(self, model, train_batch, val_batch_idx)
                    for name, value in metrics.items():
                        val_metrics[name] += value
                    num_val_batches += 1
                self.callbacks.on_validation_end(self, model)
                val_metrics = _reduce_metrics(val_metrics, num_val_batches)
                self.logger.log_metrics(val_metrics, self.step)

        self.callbacks.on_train_end(self, model)
        self.logger.finalize()

    def _resume_from_checkpoint(self, model: TrainModule, ckpt_path: Optional[str]):
        if ckpt_path is None:
            return
        checkpoint = torch.load(ckpt_path, map_location=self.device)
        model.load_state_dict(checkpoint["state_dict"])
        for i in range(len(self.optimizers)):
            self.optimizers[i].load_state_dict(checkpoint["optimizer_states"][i])
        i = 0
        for lr_scheduler in self.lr_schedulers:
            if lr_scheduler is not None:
                lr_scheduler.load_state_dict(checkpoint["lr_schedulers"][i])
                i += 1
        self.step = checkpoint["step"]
        assert checkpoint["hparams"] == model.save_hyperparameters()
        sys.stderr.write(f"#{get_rank()}: resume from checkpoint {ckpt_path}\n")

    def _init_optimizers(self, model: TrainModule):
        if self.optimizers is None:
            assert self.lr_schedulers is None
            optim_conf = model.configure_optimizers()
            optimizers, lr_schedulers = _configure_optimizers(optim_conf)
            self.optimizers = []
            for optimizer in optimizers:
                optimizer = OptimizerWrapper(
                    optimizer=optimizer,
                    gradient_clip_val=self.gradient_clip_val,
                    gradient_clip_algorithm=self.gradient_clip_algorithm,
                    use_amp=self.use_amp
                )
                self.optimizers.append(optimizer)
            self.lr_schedulers = lr_schedulers


def _configure_optimizers(optim_conf: Union[Optimizer, List, Tuple]):
    optimizers, lr_schedulers = [], []
    # single output, single optimizer
    if isinstance(optim_conf, Optimizer):
        optimizers = [optim_conf]
    # two lists, optimizer + lr schedulers
    elif (
        isinstance(optim_conf, (list, tuple))
        and len(optim_conf) == 2
        and isinstance(optim_conf[0], list)
        and all(isinstance(opt, Optimizer) for opt in optim_conf[0])
    ):
        opt, sch = optim_conf
        optimizers = opt
        lr_schedulers = sch if isinstance(sch, list) else [sch]
    # single list or tuple, multiple optimizer
    elif isinstance(optim_conf, (list, tuple)) and all(isinstance(opt, Optimizer) for opt in optim_conf):
        optimizers = list(optim_conf)
    # unknown configuration
    else:
        raise Exception("Unknown configuration for model optimizers.")
    return optimizers, lr_schedulers


def _metrics_to_scalars(metrics: Dict[str, _MetricTypes]):
    def to_item(value: Union[int, float, torch.Tensor]) -> Union[int, float]:
        if not isinstance(value, torch.Tensor):
            return value
        if value.numel() != 1:
            raise ValueError(
                f"The metric `{value}` does not contain a single element, thus it cannot be converted to a scalar."
            )
        return value.item()

    return {name: to_item(value) for name, value in metrics.items()}


def _reduce_metrics(metrics: Dict[str, _MetricTypes], num_batches: int) -> Dict[str, _MetricTypes]:
    reduced_metrics = {}
    num_batches = _all_reduce(num_batches)
    for name, value in metrics.items():
        reduced_metrics[name] = _all_reduce(value) / num_batches
    return reduced_metrics


def _all_reduce(value: _MetricTypes, op: dist.ReduceOp = dist.ReduceOp.SUM):
    if dist.is_initialized():
        value_tensor = value
        if not isinstance(value, torch.Tensor):
            value_tensor = torch.tensor(value).cuda()
        dist.all_reduce(value_tensor, op)
        return value_tensor
    return value
