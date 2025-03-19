from dataclasses import dataclass
from typing import Dict, Any


@dataclass
class Experiment:
    model_name: str
    model_conf: Dict[Any, Any]
    metrics: Dict[Any, Any]
