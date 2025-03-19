#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Fri Nov 30 00:53:38 2018

@author: artkaz
"""
#%%
from SwissArmyKnife import SwissArmyKnife, YTClient, ClickHouseClient
from datetime import datetime, date, timedelta
import pandas as pd
import numpy as np
sw = SwissArmyKnife()

import sys
reload(sys)
sys.setdefaultencoding('utf8')


yt_client = YTClient ('Hahn')
ch_settings_df = pd.read_excel('/Users/artkaz/Documents/yacloud/settings/ch_cloud_analytics_connection.xlsx') #sys.argv[1]

ch_settings = {ch_settings_df.iloc[i,:]['field']: ch_settings_df.iloc[i,:]['value'] for i in ch_settings_df.index}


#%%

ch_client = ClickHouseClient(user = ch_settings['user'],
                             passw = ch_settings['password'],
                             verify = ch_settings['verify'],
                             host = ch_settings['host']
                             )




#%%

# читаем данные, добавляем верхнеуровнеывый source
client_source = yt_client.read_yt_table('/home/cloud_analytics/clients/client_sources').rename({'source2':'source'}, axis=1)



data = yt_client.read_yt_table('/home/cloud_analytics/clients/passport_cloud_ba/2018-11-29').drop_duplicates().fillna(np.nan)

#%%
data = pd.merge(data, client_source, how='left', on='source', validate='many_to_one').rename({'source':'source2',
                                                                                              'source1':'source'}, axis=1)



#%%
data['ba_curr_state'] = data['ba_curr_state'].fillna('not_created')
data['source'] = data['source'] + '&' + data['ba_curr_state']

#%%
# Удаляем два BA - один internal, который в экселе Борлыка был протэган два раза - internal и direct_offer, другой дубликат Большого музея
data = data[~((data['billing_account_id'] == 'dn2vh38msujasdi3trv2') & (data['source'] == 'direct_offer'))]
data = data[data['billing_account_id'] != 'dn2stqm7q5cbtf37r4gf']



#Преобразуем даты
data['created_at'] = data['created_at'].apply(sw.d_from_ts)
data['ba_created_at'] = data['ba_created_at'].apply(sw.d_from_ts)
data['ba_paid_at'] = data['ba_paid_at'].apply(sw.d_from_ts)

def to_date(d):
    if not pd.isnull(d):
       return datetime.strptime(str(d), '%Y-%m-%d').date()
    else:
        return d

data['first_consumption_date'] = data['first_consumption_date'].apply(to_date)
data['first_paid_consumption_date'] = data['first_paid_consumption_date'].apply(to_date)

data['created_at'] = data['created_at'].fillna(date(2018,9,1))
data['reg_week'] = data['created_at'].apply(sw.last_monday_before)
data['reg_week'] = data['reg_week'].apply(str) + '&' + data['ba_curr_state']

# Cчитаем кол-во регистраций в разбивке по неделе регистрации и source, чтобы потом
reg_by_week  = data.groupby('reg_week',
                            as_index=False).\
                        agg({'passport_uid':'nunique'}).\
                        rename({'reg_week':'tag',
                                'passport_uid':'tag_count'}, axis=1)

reg_by_source  = data.groupby('source',
                            as_index=False).\
                        agg({'passport_uid':'nunique'}).\
                        rename({'source':'tag',
                                'passport_uid':'tag_count'}, axis=1)




#%%

#Считаем винтажи - кумулятивное кол-ва клиентов (в абсолютном и процентном выражении) достигших определенной стадии воронки на каждую дату в разбивке по неделе регистрации/источнику
#Стадии воронки:
#    1. Зарегистрировался в консоли
#    2. Создал BA
#    3. Начал потребление
#    4. Установил статус "Платное потребление"
#    5. Начал платное потребление
#
#%%
def extract_date(s_):
    s = s_.split('&')[0]
    return datetime.strptime(s, '%Y-%m-%d').date()
#%%

all_vintages_df = pd.DataFrame()

def calc_vintages( tag_field,date_field, vintage_name, id_field='billing_account_id'):
    res_df = sw.calc_vintages(df = data,
                               id_field = id_field,
                               date_field = date_field,
                               tag_field = tag_field,
                               date_start = data['reg_week'].apply(extract_date).min(),
                               date_end = date.today(),
                               prefix = '').rename({tag_field:'tag'}, axis=1)
    if tag_field=='reg_week':
        res_df = pd.merge(res_df,
                           reg_by_week,
                            how='left',
                            on='tag')
    else:
        res_df = pd.merge(res_df,
                           reg_by_source,
                            how='left',
                            on='tag')

    res_df['type'] = vintage_name
    res_df['cum_count_pct'] = res_df['cum_count'] / res_df['tag_count']
    res_df['date'] = res_df['date'].apply(lambda d: str(d) + ' 00:00:00')
    return res_df



#облаков создано в разбивке по источнику
all_vintages_df = all_vintages_df.append(calc_vintages('source','created_at', 'cloud_created', 'passport_uid'))
#BA создано в разбивке по дате регистрации
all_vintages_df = all_vintages_df.append(calc_vintages('reg_week','ba_created_at', 'ba_created'))
#BA создано в разбивке по источнику
all_vintages_df = all_vintages_df.append(calc_vintages('source','ba_created_at', 'ba_source_created'))
#Первое потребление в разбивке по дате регистрации
all_vintages_df = all_vintages_df.append(calc_vintages('reg_week','first_consumption_date', 'ba_first_consumption'))
#Первое потребление в разбивке по источнику
all_vintages_df = all_vintages_df.append(calc_vintages('source','first_consumption_date', 'ba_source_first_consumption'))
#Платный статус в разбивке по дате регистрации
all_vintages_df = all_vintages_df.append(calc_vintages('reg_week','ba_paid_at', 'ba_paid'))
#Платный статус в разбивке по источнику
all_vintages_df = all_vintages_df.append(calc_vintages('source','ba_paid_at', 'ba_source_paid'))
#Первое платное потребление в разбивке по дате регистрации
all_vintages_df = all_vintages_df.append(calc_vintages('reg_week','first_paid_consumption_date', 'ba_first_paid_consumption'))
#Первое платное потребление в разбивке по источнику
all_vintages_df = all_vintages_df.append(calc_vintages('source','first_paid_consumption_date', 'ba_source_first_paid_consumption'))


#Дополнительно считаем кол-во активных BA, для всех стадий воронки начиная с "Первое потребление"

consumption_daily = pd.merge(yt_client.read_yt_table('/home/cloud_analytics/import/billing2/billing_records_daily/2018-10-31'),
                             data[['billing_account_id', 'source', 'ba_paid_at']].groupby('billing_account_id', as_index=False).agg({'source': lambda s: s.values[0],
                                                                                                                                  'ba_paid_at': lambda s: s.values[0]}), how='left', on='billing_account_id', validate='many_to_one')

ba_active = consumption_daily.groupby(['date', 'source'], as_index=False).agg({'billing_account_id':'nunique'}).rename({'source':'tag',
                                                                                                                      'billing_account_id':'cum_count'   }, axis=1)
ba_active['type'] = 'ba_source_active_consumption'
ba_active['date'] = ba_active['date'] + ' 00:00:00'
ba_active['tag_count'] = ba_active['cum_count']
ba_active['cum_count_pct'] = 1

all_vintages_df = all_vintages_df.append(ba_active)

ba_active_paid_status = consumption_daily[~pd.isnull(consumption_daily['ba_paid_at'])].groupby(['date', 'source'], as_index=False).agg({'billing_account_id':'nunique'}).rename({'source':'tag',
                                                                                                                      'billing_account_id':'cum_count'   }, axis=1)
ba_active_paid_status['type'] = 'ba_source_active_paid_status'
ba_active_paid_status['date'] = ba_active_paid_status['date'] + ' 00:00:00'
ba_active_paid_status['tag_count'] = ba_active_paid_status['cum_count']
ba_active_paid_status['cum_count_pct'] = 1

all_vintages_df = all_vintages_df.append(ba_active_paid_status)


ba_active_paid_consumption = consumption_daily[consumption_daily['paid_consumption']>0].groupby(['date', 'source'], as_index=False).agg({'billing_account_id':'nunique'}).rename({'source':'tag',
                                                                                                                      'billing_account_id':'cum_count'   }, axis=1)
ba_active_paid_consumption['type'] = 'ba_source_active_paid_consumption'
ba_active_paid_consumption['date'] = ba_active_paid_consumption['date'] + ' 00:00:00'
ba_active_paid_consumption['tag_count'] = ba_active_paid_consumption['cum_count']
ba_active_paid_consumption['cum_count_pct'] = 1

all_vintages_df = all_vintages_df.append(ba_active_paid_consumption)

all_vintages_df['ba_curr_state'] = all_vintages_df['tag'].apply(lambda s: s.split('&')[1])
all_vintages_df['tag'] = all_vintages_df['tag'].apply(lambda s: s.split('&')[0])

ch_client.write_df_to_table(all_vintages_df, 'cloud_analytics.trials_by_weeks')







#%%

#Считаем события за неделю - сколько клиентов достигли соответствующих стадий воронки за каждую неделю
def last_monday_before_(d):
    if not pd.isnull(d):
       return sw.last_monday_before(datetime.strptime(str(d), '%Y-%m-%d').date())
    else:
        return d

data['reg_week'] = data['reg_week'].apply(lambda s: datetime.strptime(s.split('&')[0], '%Y-%m-%d').date())


week_events = {}
week_events['reg_week'] = data.groupby(['source', 'reg_week'], as_index=False).agg({'passport_uid':'nunique'}).rename({'reg_week':'week'}, axis=1)

for col in ['ba_created_at',
            'first_consumption_date',
            'ba_paid_at',
            'first_paid_consumption_date']:



    data[col + '_week'] = data[col].apply(last_monday_before_)

    week_events[col] = data.groupby(['source', col+'_week'], as_index=False).agg({'billing_account_id':'count'}).rename({col+'_week': 'week',
                                                                                                               'billing_account_id': col}, axis=1)

week_events_df = pd.DataFrame(columns=['week', 'source'])

for col in week_events.keys():
    week_events_df = pd.merge(week_events_df, week_events[col], how='outer', on=['source', 'week'])

week_events_df['week_str'] = week_events_df['week'].apply(sw.return_week)

week_events_df['week'] = week_events_df['week'].apply(lambda d: str(d) + ' 00:00:00' )

week_events_df.rename({ 'first_paid_consumption_date': 'first_paid_consumption',
                        'ba_paid_at': 'ba_paid',
                        'ba_created_at' : 'ba_created',
                        'passport_uid' : 'registered_in_console',
                        'first_consumption_date': 'first_consumption'
                            }, axis=1, inplace=True)


week_events_df['ba_curr_state'] = week_events_df['source'].apply(lambda s: s.split('&')[1])
week_events_df['source'] = week_events_df['source'].apply(lambda s: s.split('&')[0])
#%%
ch_client.write_df_to_table(week_events_df, 'cloud_analytics.week_events')


#%%
#считаем статистику по стадиям воронки для когорт "дата регистрации" и "источник"
data['source'] = data['source'].apply(lambda s: s.split('&')[0])

vintages_df = data[:].groupby(['reg_week','source','ba_curr_state'], as_index=False).agg({'passport_uid':'nunique',
                                              'billing_account_id':'count',
                                              'first_consumption_date': 'count',
                                              'ba_paid_at': 'count',
                                              'first_paid_consumption_date':'count'
                                                }).rename({'passport_uid':'activated_count',
                                                           'billing_account_id': 'created_ba_count',
                                                           'first_consumption_date': 'consumes_trial_count',
                                                           'ba_paid_at':'ba_paid_count',
                                                           'first_paid_consumption_date': 'consumes_paid_count'}, axis=1)





def parse_date(d):
    try:
        return sw.d_from_str(d)
    except Exception as e:
        return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

def return_week(d):
    try:
        w = ''
        w += sw.d_from_str(d).strftime('%m-%d')
        end_of_week = sw.d_from_str(d) + timedelta(days=6)
        return w + ' to ' + end_of_week.strftime('%m-%d')

    except Exception as e:
        #print (e)
        return d


vintages_df['reg_week_monday'] = vintages_df['reg_week'].apply(parse_date)
vintages_df['reg_week'] = vintages_df['reg_week'].apply(return_week)

ch_client.write_df_to_table(vintages_df.iloc[:,:], 'cloud_analytics.trials_stats')


#%%

#%%

#ddd

