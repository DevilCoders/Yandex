from functools import wraps
import logging
from platform import python_version
import random
import sys
from typing import Any, Callable, Optional, Tuple

import numpy as np
import torch
import torch.nn.functional as F


def get_rank() -> int:
    if torch.distributed.is_initialized():
        return torch.distributed.get_rank()
    return 0


def rank_zero_only(fn: Callable) -> Callable:
    @wraps(fn)
    def wrapped_fn(*args: Any, **kwargs: Any) -> Optional[Any]:
        if get_rank() == 0:
            return fn(*args, **kwargs)
        return None

    return wrapped_fn


@rank_zero_only
def _init_logging():
    logging.basicConfig(
        level=logging.DEBUG,
        stream=sys.stdout,
        format="%(levelname)s: %(asctime)s    %(message)s"
    )


_init_logging()
log = logging.getLogger(__name__)


def _info(*args: Any, stacklevel: int = 2, **kwargs: Any) -> None:
    if python_version() >= "3.8.0":
        kwargs["stacklevel"] = stacklevel
    log.info(*args, **kwargs)


def _debug(*args: Any, stacklevel: int = 2, **kwargs: Any) -> None:
    if python_version() >= "3.8.0":
        kwargs["stacklevel"] = stacklevel
    log.debug(*args, **kwargs)


@rank_zero_only
def rank_zero_info(*args: Any, stacklevel: int = 4, **kwargs: Any) -> None:
    """Function used to log info-level messages only on rank 0."""
    _info(*args, stacklevel=stacklevel, **kwargs)


@rank_zero_only
def rank_zero_debug(*args: Any, stacklevel: int = 4, **kwargs: Any) -> None:
    """Function used to log debug-level messages only on rank 0."""
    _debug(*args, stacklevel=stacklevel, **kwargs)


def seed_everything(seed: int):
    rank_zero_info(f"global seed set to {seed}")
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed_all(seed)


def mask_from_lengths(lengths: torch.Tensor) -> torch.Tensor:
    ids = torch.arange(0, lengths.max()).to(lengths)
    mask = torch.lt(ids, lengths.unsqueeze(1))
    return mask


def get_segments(x: torch.Tensor,
                 start_indices: torch.Tensor,
                 segment_size: int) -> torch.Tensor:
    if x.size(2) < segment_size:
        pad = segment_size - x.size(2)
        x = F.pad(x, (0, pad), "constant", 0)
    batch_size, dim, _ = x.size()
    segments = x.new_zeros(batch_size, dim, segment_size)
    for i, start_idx in enumerate(start_indices):
        segments[i] = x[i, :, start_idx: start_idx + segment_size].clone()
    return segments


def get_random_segments(x: torch.Tensor,
                        lengths: torch.Tensor,
                        segment_size: int) -> Tuple[torch.Tensor, torch.Tensor]:
    if x.size(2) < segment_size:
        pad = segment_size - x.size(2)
        x = F.pad(x, (0, pad), "constant", 0)
    batch_size = x.size(0)
    max_start_idx = (lengths - segment_size).clamp(0)
    start_indices = (torch.rand(batch_size).to(x) * max_start_idx).long()
    segments = get_segments(x, start_indices, segment_size)
    return segments, start_indices


def nccl_barrier_on_cpu():
    if not torch.distributed.is_initialized():
        return
    with torch.no_grad():
        fake = torch.randn([16])
        fake = fake.cuda()
        torch.distributed.all_reduce(fake)
        fake = fake.cpu()
