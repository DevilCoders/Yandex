from typing import Optional

import torch
import torch.cuda.amp as amp
import torch.optim as optim


class OptimizerWrapper:
    def __init__(self,
                 optimizer: optim.Optimizer,
                 gradient_clip_val: Optional[float] = None,
                 gradient_clip_algorithm: Optional[str] = None,
                 use_amp: bool = False):
        self._optimizer = optimizer
        self.gradient_clip_val = gradient_clip_val
        self.gradient_clip_algorithm = gradient_clip_algorithm
        self.use_amp = use_amp
        self._scaler = amp.GradScaler(enabled=use_amp)

    def __call__(self, loss):
        self._optimizer.zero_grad()
        self._scaler.scale(loss).backward()
        if self.gradient_clip_val is not None:
            self._scaler.unscale_(self._optimizer)
            if self.gradient_clip_algorithm is None or self.gradient_clip_algorithm == "norm":
                torch.nn.utils.clip_grad_norm_(self._optimizer.param_groups[0]["params"], self.gradient_clip_val)
            else:
                assert self.gradient_clip_algorithm == "value"
                torch.nn.utils.clip_grad_value_(self._optimizer.param_groups[0]["params"], self.gradient_clip_val)
        self._scaler.step(self._optimizer)
        self._scaler.update()

    @property
    def optimizer(self):
        return self._optimizer

    def get_lr(self):
        return self._optimizer.param_groups[0]["lr"]

    def state_dict(self):
        return self._optimizer.state_dict()

    def load_state_dict(self, state):
        self._optimizer.load_state_dict(state)
