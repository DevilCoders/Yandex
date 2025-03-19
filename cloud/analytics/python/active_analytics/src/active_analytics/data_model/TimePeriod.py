from dataclasses import dataclass

import pandas as pd



@dataclass
class TimePeriod:
    period_from : pd.Timestamp
    period_to : pd.Timestamp

    def __str__(self):
        return f'[{self.period_from.date()}, {self.period_to.date()})'
