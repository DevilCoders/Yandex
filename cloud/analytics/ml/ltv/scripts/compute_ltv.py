import json
import numpy as np
import pandas as pd
import statsmodels.api as sm
import matplotlib.pyplot as plt
import logging.config
from textwrap import dedent
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from ltv.utils import config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def get_predictions_for_channel(data, channel, weight, preds_len=200, window=3):
    df = data[(data['channel'] == channel)&(data['channel_weight_name'] == weight)]
    pivot_coeff = df.pivot(index='start_month', columns='week', values='week_consumption')
    coeff = {}
    for i in sorted(pivot_coeff.columns)[:-1]:
        coeff[i] = (pivot_coeff[i+1]/pivot_coeff[i]).dropna()[-window:].mean()
    for i in sorted(pivot_coeff.columns)[:-1]:
        pivot_coeff.loc[(pivot_coeff[i+1].isna() & ~pivot_coeff[i].isna()), i+1] = pivot_coeff.loc[(pivot_coeff[i+1].isna() & ~pivot_coeff[i].isna()), i]*coeff[i]
    tmp = pd.merge(df, pivot_coeff.reset_index().melt('start_month'), how='right', on=['week','start_month'])


    tmp['max_people'] = tmp.groupby('start_month')['people_count'].transform('max')
    tmp['week_consumption'] = tmp['week_consumption']/tmp['max_people']
    tmp = tmp.groupby('week')['week_consumption'].mean().reset_index()
    
    f = tmp.sort_values('week')['week_consumption']
    f = np.hstack((f.values, np.array([np.nan for _ in range(preds_len)])))

    model=sm.tsa.statespace.SARIMAX(
        endog=f,
        exog = list(range(len(f))),
        order=(1, 0, 1),
        seasonal_order=(0, 0, 0, 0), # S
        trend=(1,1,0)
    ).fit(disp=-1)

    a = model.predict()[-preds_len:]
    a = a*(a>0)
    for x in a:
        tmp = tmp.append({'week': tmp.iloc[-1, 0] + 1,
                   'week_consumption_pred': x}, ignore_index=True)
    tmp.loc[~tmp['week_consumption'].isna(), 'week_consumption'] = tmp['week_consumption'].rolling(1000, min_periods=0).sum()
    tmp.loc[~tmp['week_consumption_pred'].isna(), 'week_consumption_pred'] = (tmp['week_consumption_pred'].rolling(1000, min_periods=0).sum()) + tmp['week_consumption'].max()
    tmp['week'] = tmp['week'].astype(int)
    tmp['channel'] = channel
    tmp['channel_weight_name'] = weight
    return tmp[['week', 'channel', 'channel_weight_name', 'week_consumption', 'week_consumption_pred']]


def main() -> None:
    data_path = '//home/cloud_analytics/ml/ltv/data'
    result_path = "//home/cloud_analytics/ml/ltv/"
    yt_adapter = YTAdapter()
    market_channels = config['market_channels']
    channel_weights = config['weights']


    data = yt_adapter.read_table(data_path, to_pandas=True)
    result = get_predictions_for_channel(data, market_channels[0], channel_weights[0])
    for i, channel in enumerate(market_channels):
        for j, weight in enumerate(channel_weights):
            if (i, j) != (0, 0):
                tmp = get_predictions_for_channel(data, channel, weight)
                result = pd.concat([result, tmp])

    yt_schema = [
        {'name': 'week', 'type': 'int64'},
        {'name': 'channel', 'type': 'string'},
        {'name': 'channel_weight_name', 'type': 'string'},
        {'name': 'week_consumption', 'type': 'double'},
        {'name': 'week_consumption_pred', 'type': 'double'}
        ]
    yt_adapter.save_result(result_path + "predictions", yt_schema, result, append=False)

    ltv_values = result.groupby(['channel', 'channel_weight_name'])['week_consumption_pred'].max().reset_index()
    yt_schema = [
        {'name': 'channel', 'type': 'string'},
        {'name': 'channel_weight_name', 'type': 'string'},
        {'name': 'week_consumption_pred', 'type': 'double'}
        ]
    yt_adapter.save_result(result_path + "ltv_by_channel", yt_schema, ltv_values, append=False)


    with open('output.json', 'w+') as f:
        json.dump({"output" : result_path}, f)

if __name__ == "__main__":
    main()
