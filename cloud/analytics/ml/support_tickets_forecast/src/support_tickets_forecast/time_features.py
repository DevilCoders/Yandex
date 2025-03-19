import typing as tp
from datetime import datetime, timedelta

import numpy as np
import pandas as pd

from clan_tools.data import df_holiday_in_russia


DAYS_OF_WEEK = {'MON': 0, 'TUE': 1, 'WED': 2, 'THU': 3, 'FRI': 4, 'SAT': 5, 'SUN': 6}
DAYS_OF_WEEK_REV = {value: key for key, value in DAYS_OF_WEEK.items()}


def safe_resample_weekly(dts: tp.Union[pd.Series, pd.DataFrame], sampling: tp.Optional[str] = None) -> tp.Union[pd.Series, pd.DataFrame]:
    dts = dts.copy()
    sampling = sampling or 'W-'+DAYS_OF_WEEK_REV[dts.index.max().weekday()]
    weekday = pd.Series(dts.index.weekday)
    end_num = DAYS_OF_WEEK[sampling[-3:]]
    start_num = (end_num + 1) % 7
    end_ind = weekday[weekday == end_num].index[-1]
    start_ind = weekday[weekday == start_num].index[0]

    dts = dts.iloc[start_ind:end_ind+1].resample(sampling).sum()
    return dts


def get_date_category_features(date_from: str, date_to: str) -> pd.DataFrame:
    """Generate date features

    :param date_from: datetime str in '%Y-%m-%d' format to begin from
    :param date_to: datetime str in '%Y-%m-%d' format to end with
    :return: DataFrame with date as index and columns 'day_of_week', 'day_of_month', 'month', 'holiday'
    """
    df_date_features = pd.DataFrame()
    df_date_features['date'] = pd.date_range(date_from, date_to)

    df_date_features['day_of_week'] = df_date_features['date'].dt.weekday
    df_date_features['day_of_month'] = df_date_features['date'].dt.day
    df_date_features['month'] = df_date_features['date'].dt.month
    df_holiday_in_russia['date'] = pd.to_datetime(df_holiday_in_russia['date'])

    df_date_features = df_date_features.merge(df_holiday_in_russia, on='date', how='left')

    return df_date_features


def get_date_sin_cos_features(date_from: str, date_to: str) -> pd.DataFrame:
    """Generate date features

    :param date_from: datetime str in '%Y-%m-%d' format to begin from
    :param date_to: datetime str in '%Y-%m-%d' format to end with
    :return: DataFrame with date as index and columns as sin and cosine of 'day_of_week', 'day_of_month', 'month' and binary 'holiday'
    """
    df_date_features = get_date_category_features(date_from, date_to)

    df_date_features['day_of_week_sin'] = np.sin(df_date_features['day_of_week'] * 2 * np.pi / 7)
    df_date_features['day_of_week_cos'] = np.cos(df_date_features['day_of_week'] * 2 * np.pi / 7)

    df_date_features['day_of_month_sin'] = np.sin(df_date_features['day_of_month'] * 2 * np.pi / 31)
    df_date_features['day_of_month_cos'] = np.cos(df_date_features['day_of_month'] * 2 * np.pi / 31)

    df_date_features['month_sin'] = np.sin(df_date_features['month'] * 2 * np.pi / 12)
    df_date_features['month_cos'] = np.cos(df_date_features['month'] * 2 * np.pi / 12)

    df_date_features = df_date_features.drop(columns=['day_of_week', 'day_of_month', 'month'])

    return df_date_features


def get_date_dummy_features(date_from: str, date_to: str) -> pd.DataFrame:
    """Generate date features

    :param date_from: datetime str in '%Y-%m-%d' format to begin from
    :param date_to: datetime str in '%Y-%m-%d' format to end with
    :return: DataFrame with date as index and dummy columns of 'day_of_week', 'day_of_month', 'month' and binary 'holiday'
    """
    temp_date_to = (datetime.strptime(date_from, '%Y-%m-%d') + timedelta(days=366)).strftime('%Y-%m-%d')
    temp_date_to = max(temp_date_to, date_to)
    df_date_features = get_date_category_features(date_from, temp_date_to)
    datetime_columns = ['day_of_week', 'day_of_month', 'month']

    for colname in datetime_columns:
        temp = pd.get_dummies(df_date_features[colname])
        temp.columns = [f'{colname}_{cc}' for cc in temp.columns]
        df_date_features = pd.concat([df_date_features, temp], axis=1)

    df_date_features = df_date_features.drop(columns=datetime_columns)
    df_date_features = df_date_features.sort_values('date')
    df_date_features = df_date_features[df_date_features['date'] <= datetime.strptime(date_to, '%Y-%m-%d')]

    return df_date_features


def get_date_sin_cos_features_weekly(date_from: str, date_to: str, num_periods: int = 30) -> pd.DataFrame:
    date_to = (datetime.strptime(date_to, '%Y-%m-%d') + timedelta(days=7*num_periods)).strftime('%Y-%m-%d')
    df_date_features = get_date_category_features(date_from, date_to)
    df_date_features = df_date_features.set_index('date')

    sampling = 'W-'+DAYS_OF_WEEK_REV[df_date_features.index.max().weekday()]
    weekday = pd.Series(df_date_features.index.weekday)
    end_num = DAYS_OF_WEEK[sampling[-3:]]
    start_num = (end_num + 1) % 7
    end_ind = weekday[weekday == end_num].index[-1]
    start_ind = weekday[weekday == start_num].index[0]
    df_date_features = df_date_features.iloc[start_ind:end_ind+1]

    df_date_features = df_date_features.resample(sampling).agg({
        'day_of_month': lambda x: x[-1],
        'month': lambda x: x[-1],
        'holiday': sum
    })

    df_date_features['day_of_month_sin'] = np.sin(df_date_features['day_of_month'] * 2 * np.pi / 31)
    df_date_features['day_of_month_cos'] = np.cos(df_date_features['day_of_month'] * 2 * np.pi / 31)

    df_date_features['month_sin'] = np.sin(df_date_features['month'] * 2 * np.pi / 12)
    df_date_features['month_cos'] = np.cos(df_date_features['month'] * 2 * np.pi / 12)

    df_date_features['holiday'] = df_date_features['holiday'].shift(-num_periods) / 7 - 0.5
    df_date_features = df_date_features.drop(columns=['day_of_month', 'month']).dropna()

    return df_date_features


__all__ = ['safe_resample_weekly', 'get_date_category_features', 'get_date_sin_cos_features', 'get_date_dummy_features', 'get_date_sin_cos_features_weekly']
