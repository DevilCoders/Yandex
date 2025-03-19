import numpy as np
from clan_tools.data_model.tracker.TrackerTicket import TrackerTicket
from clan_tools.utils.time import rus_format_date, curr_utc_date_iso
from datetime import datetime, timedelta
from dataclasses import asdict
import logging 
from clan_tools.utils.timing import timing
from active_analytics.data_adapter.DailyConsumptionAdapter import DailyConsumptionAdapter
from clan_tools.data_adapters.TrackerResultAdapter import TrackerResultAdapter
from typing import Dict
import pandas as pd
from active_analytics.data_adapter import DailyConsumptionLogAdapter
from textwrap import dedent
logger = logging.getLogger(__name__)

class DailyConsumptionNotifier:
    def __init__(self, daily_cons_adapter: DailyConsumptionAdapter, tracker_adapter: TrackerResultAdapter,
                 default_responsibles: Dict[str, str], default_followers: Dict[str, str], 
                 queue:str, log_adapter:DailyConsumptionLogAdapter, remind_period:int=30
                 ):
        self._cons_adapter = daily_cons_adapter
        self._result_adapter = tracker_adapter
        self._responsibles = default_responsibles
        self._followers = default_followers
        self._queue = queue
        self._log_adapter:DailyConsumptionLogAdapter = log_adapter
        self._remind_period = remind_period

    def _map_ticket(self, consumption_drop):
        assignee = np.nan
        followers = np.nan
        if not pd.isnull(consumption_drop['sales_name']):
            assignee = consumption_drop['sales_name']
        else:
            if consumption_drop['segment'] in self._responsibles:
                assignee = self._responsibles[consumption_drop['segment']]
        if (consumption_drop['service_name'] in self._followers):
            followers = self._followers[consumption_drop['service_name']]
        return assignee, followers

    def _send(self, df: pd.DataFrame, date:datetime):
        assignee = df.assignee.values[0]
        segment = df.segment.values[0]
        message = ''
        followers = df.followers[df.followers.notna()].unique().tolist()

        def _format_cons(cons1, cons2, pct):
            return (cons1.fillna(0) - cons2.fillna(0)).astype(int).astype(str) \
                    + '₽ (' + ((pct*100).fillna(0).astype(int)).astype(str) + '%)'
        
        chart_str = dedent('''{{{{iframe frameborder="0" width="100%" height="400px" src="https://charts.yandex-team.ru/preview/editor/wklvl5ozsw2cs?_embedded=1&date_from={date_from}&date_to={date_to}&service={service}''')

        services = df.service_name.unique()
        for service in services:
            service_message = '%%(markdown)\n<br><br>\n '
            service_message += "**По всем сервисам суммарно** \n\n" if service == 'total' \
                else f"**По сервису {service.replace('_', ' ').upper()}** \n\n"
            hide_columns = ['assignee', 'followers',
                            'sales_name', 'segment', 'service_name']
            df_to_show: pd.DataFrame = df.drop(hide_columns, axis=1)[df.service_name == service]\
                .copy().reset_index(drop=True)
            df_to_show['paid'] = _format_cons(
                df_to_show.paid_cons_cur, df_to_show.paid_cons_1_3,  df_to_show.paid_diff)
            df_to_show['total'] = _format_cons(
                df_to_show.total_cons_cur, df_to_show.total_cons_1_3,  df_to_show.total_diff)
            df_to_show = df_to_show[[
                'billing_account_id', 'account_name', 'paid', 'total']]
            billing_accounts = df_to_show['billing_account_id'].unique()

            service_chart_str = chart_str.format(service=service, 
                                                date_to=date.isoformat(),
                                                date_from=(date - timedelta(days=30)).isoformat())
            for billing_account in billing_accounts:
                service_chart_str += f'&billing_account_ids={billing_account}'

            df_to_show.columns = ['Биллинг аккаунт', 'Имя аккаунта',
                                  'Снижение платного потреб.', 'Снижение всего потреб.']
            service_message += df_to_show.to_markdown(showindex=False)   

            message += service_message + '\n\n%%\n\n'+ service_chart_str + '"}}\n\n' 
                     
          

        summary = f'{rus_format_date(date)} упало потребление у следующих аккаунтов'
        ticket = TrackerTicket(queue=self._queue,
                               description=message,
                               type=2,
                               summary=summary,
                               assignee='danilapavlov',
                               followers=['bakuteev'],
                               tags=[segment.lower().replace(' ', '_')] + services.tolist())
        if not df_to_show.empty and self._result_adapter:
            resp = self._result_adapter.send_data(asdict(ticket))

    def _check_logs(self, cons_df):
        if self._log_adapter:
            logged_accounts = self._log_adapter.get_last_logged_ba(num_last_days=self._remind_period)
            if logged_accounts is not None:
                cons_df = pd.merge(cons_df, logged_accounts, on='billing_account_id', indicator=True, how='left')
                cons_df = cons_df[cons_df['_merge']=='left_only']

        log_ba = cons_df[['billing_account_id']].copy()
        log_ba.drop_duplicates(inplace=True)

        if not log_ba.empty and self._log_adapter is not None:
            self._log_adapter.log_data(log_ba)
        return cons_df




    @timing
    def notify(self, date:datetime=None):
        if date is None:
            date = datetime.now().date() - timedelta(days=1)
        cons_df = self._cons_adapter.consumption(date)
        cons_df = self._check_logs(cons_df)
        if not cons_df.empty:
            logger.info(cons_df)
            cons_df = cons_df.assign(assignee=np.nan, followers=np.nan)
            send_to = cons_df.apply(self._map_ticket, axis=1, result_type='expand')
            cons_df[['assignee', 'followers']] = send_to
            cons_df.groupby('assignee').apply(self._send, date)
        return cons_df


    def notify_over_period(self, last_date:datetime, num_days_before:int=3):
        date_list = [last_date - timedelta(days=x) for x in range(num_days_before)][::-1]
        for date in date_list:
            self.notify(date)