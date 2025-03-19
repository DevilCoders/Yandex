from datetime import datetime
import pandas as pd
import numpy as np
from active_analytics.data_model.TimePeriod import TimePeriod
from clan_tools.data_model.tracker.TrackerTicket import TrackerTicket
TICKET_FORMAT_STR = '''%%(markdown)
{markdown}
%%
'''

class ComparedConsumption(pd.DataFrame):
   
    _metadata = ['period_before_last', 'last_period']

    

    def __init__(self, data: np.array, index=None, columns=None, dtype=None, copy=True, period_before_last: TimePeriod = None,
                 last_period: TimePeriod = None,):
        super().__init__( data=data,
                                index=index,
                                columns=columns,
                                dtype=dtype,
                                copy=copy)
        self.period_before_last = period_before_last
        self.last_period = last_period
        

    @property
    def _constructor(self):
        return ComparedConsumption

   

    def to_ticket(self, queue):

        self.reset_index(drop=True, inplace=True)
        self.index = self.index + 1


        self  = self.rename(columns={'period_before_last_cons': str(self.period_before_last),
                                            'last_period_cons': str(self.last_period)})
        markdown_table = TICKET_FORMAT_STR.format(markdown=self.to_markdown())

        ticket = TrackerTicket(queue= queue,
                    summary = (f"Падение потребления за период {self.period_before_last} "
                            f"по сравнению с периодом {self.last_period}"),
            type = 2,
            description =  markdown_table)
        return ticket