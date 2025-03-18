import io
from datetime import timedelta

import pytest
import pandas as pd

from library.python import resource

from antiadblock.tasks.tools.calendar import Calendar
from antiadblock.tasks.money_monitoring.lib.data_checks import check_money_drop


def prepare_data(data_file):
    holidays = Calendar().get_nonwork_days(force_fallback=True)

    data = pd.read_csv(io.BytesIO(data_file), sep=';')
    data.loc[:, 'fielddate'] = pd.to_datetime(data['fielddate'], format='%d.%m.%Y %H:%M')
    data.loc[:, 'money'] = pd.to_numeric(data['money'].apply(lambda s: s.replace(',', '.')))
    data.loc[:, 'aab_money'] = pd.to_numeric(data['aab_money'].apply(lambda s: s.replace(',', '.')))
    data.loc[:, 'unblock'] = pd.to_numeric(data['unblock'].apply(lambda s: s.replace(',', '.')))
    data['is_holiday'] = data.apply(lambda d: d['fielddate'].strftime('%Y-%m-%d') in holidays, axis=1)
    data.set_index(['fielddate'], inplace=True)

    # Проверяем актуальность данных по выходным и праздникам
    last_ts = data.index.max()
    last_weekend = last_ts - timedelta(days=last_ts.weekday() + 1)
    assert last_weekend.strftime('%Y-%m-%d') in holidays

    return data


@pytest.mark.parametrize('prefix, expected_status', [['tp', 'CRIT'], ['tn', 'OK']])
def test_money_drop(prefix, expected_status):
    for data_file in resource.itervalues(prefix):
        data = prepare_data(data_file)
        assert check_money_drop(data)['status'] == expected_status
