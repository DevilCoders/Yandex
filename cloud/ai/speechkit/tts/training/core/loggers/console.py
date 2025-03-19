from typing import Dict, Optional

from core.loggers.base import Logger
from core.utils import rank_zero_info, rank_zero_only


class ConsoleLogger(Logger):
    def __init__(self):
        super().__init__()

    @rank_zero_only
    def log_metrics(self, metrics: Dict[str, float], step: Optional[int] = None):
        log_str = prefix = ""
        for key, value in metrics.items():
            if not (key.startswith("train/") or key.startswith("val/")):
                continue
            prefix, key = key.split("/")
            if not log_str:
                log_str = f"step: {step}"
                log_str = f"\n\nValidation\n{log_str}" if prefix == "val" else log_str
            log_str += f"\t{key}: {value:.4f}"
        if prefix == "val":
            log_str += "\n"
        if log_str:
            rank_zero_info(log_str)
