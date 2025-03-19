import pandas as pd

from .prepared_data import holiday_in_russia_list
df_holiday_in_russia = pd.DataFrame(holiday_in_russia_list, columns=['date', 'holiday'])


__all__ = ['df_holiday_in_russia']
