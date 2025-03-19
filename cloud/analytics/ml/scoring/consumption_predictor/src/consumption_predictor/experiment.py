from dataclasses import dataclass
from typing import Dict


@dataclass
class Metrics:
    mean_rec: float
    mean_prec: float
    std_rec: float
    std_prec: float
    dataset_info: Dict[str, float] = None
    feature_importances: Dict[str, float] = None
