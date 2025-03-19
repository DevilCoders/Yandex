# data is taken from https://data.gov.ru/opendata/7708660670-proizvcalendar

import os
import pandas as pd


def main() -> None:
    dirname = os.path.dirname(__file__)

    # read dataframe formatted
    colnames = ['year'] + [str(i) for i in range(1, 13)]
    df_prod_cal = pd.read_csv(f'{dirname}/raw_data.csv', skiprows=1, header=None, usecols=range(13))
    df_prod_cal.columns = colnames

    # create long table with holiday date
    df_holiday = pd.melt(df_prod_cal, id_vars=['year'], value_vars=colnames[1:], var_name='month', value_name='day')
    df_holiday['day'] = df_holiday['day'].str.replace('*', '', regex=False).str.split(',')
    df_holiday = df_holiday.explode('day').astype(str)
    df_holiday['month'] = df_holiday['month'].str.zfill(2)
    df_holiday['day'] = df_holiday['day'].str.zfill(2)
    df_holiday['holiday'] = df_holiday.apply(lambda x: '-'.join(x), axis=1)
    df_holiday = df_holiday[['holiday']]

    # create long table with all dates
    df_date = pd.DataFrame()
    df_date['date'] = pd.date_range('1999-01-01', '2025-12-31').strftime('%Y-%m-%d')

    # result table
    df_res = df_date.merge(df_holiday, left_on='date', right_on='holiday', how='left')
    df_res['holiday'] = df_res['holiday'].notna().astype(int)
    holiday_list = df_res.values.tolist()
    with open('src/clan_tools/data/holidays_in_russia/prepared_data.py', 'w') as fout:
        fout.write('holiday_in_russia_list = [\n')
        for row in holiday_list:
            fout.write(' '*4+str(row)+',\n')
        fout.write(']\n')


if __name__ == '__main__':
    main()
